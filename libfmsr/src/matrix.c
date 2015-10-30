/**
  * @file matrix.c
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Implements various matrix operations.
  * **/

/* ===================================================================
Copyright (c) 2013, Henry C. H. Chen
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

  - Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

  - Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.

  - Neither the name of the Chinese University of Hong Kong nor the
    names of its contributors may be used to endorse or promote
    products derived from this software without specific prior written
    permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=================================================================== */


#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "matrix.h"
#include "misc.h"

/*  -----------------------------------  */
/* | prototypes for internal functions | */
/*  -----------------------------------  */
/** Find needle in haystack[].
 *  Returns index of needle, or -1 if not found */
static int _in(gf needle, gf *haystack, gf size);


/** Combination for choosing k objects from n. Objects in ex[] are excluded.
 *  Forms the next combination from comb if possible and stores it back in comb,
 *  or returns 0 if all combinations have been exhausted */
static int _next_comb(gf *comb, gf n, gf k, gf *ex, gf num_ex);


/** Gaussian elimination on A (used in rank calculation).
 *  A is an n x m matrix.  Returns rank(A) on success. */
static int _gaussian_elimination(gf *A, gf n, gf m);


/** Gauss-Jordan elimination on A (used in calculating inverse).
 *  A is an n x m matrix.  Returns rank(A) on success. */
static int _gauss_jordan(gf *A, gf n, gf m);


/** Multi-threading stuff for matrix_mul_p() */
static void *_mul(void *args);
typedef struct _thread_args {
  gf *A, *B, *C, n, k;
  size_t m, m_sub;
} _thread_args;


/*  ---------------------------  */
/* | the core matrix functions | */
/*  ---------------------------  */
void matrix_mul(gf *A, gf *B, gf *C, gf n, gf k, size_t m)
{
  gf *pA=A, *pB=B, *pC=C;
  memset(pC, 0, n*m);
  for (gf i=0; i<n; i++, pB=B, pC+=m) {
    for (gf j=0; j<k; j++, pA++, pB+=m) {
      gf_mulxor_bytes(pB, m, *pA, pC);
    }
  }
}


void matrix_mul_p(gf *A, gf *B, gf *C, gf n, gf k, size_t m, int num_threads)
{
  if (num_threads > m) { num_threads = m; }
  size_t m_sub = m/num_threads;
  size_t leftover = m - num_threads*m_sub;

  // split matrix multiplication between "num_threads" threads
  // each thread is responsible for calculating "m_sub" columns of C

  _thread_args *tas = safe_talloc(_thread_args, num_threads);
  for (int i=0; i<=leftover; i++) {
    tas[i] = (_thread_args){ A, B + i*(m_sub+1), C + i*(m_sub+1),
                             n, k, m, m_sub + (i!=leftover) };
  }
  for (int i=leftover+1; i<num_threads; i++) {
    tas[i] = (_thread_args){ A, B + i*m_sub + leftover, C + i*m_sub + leftover,
                             n, k, m, m_sub };
  }

  memset(C, 0, n*m);
  pthread_t *tid = safe_talloc(pthread_t, num_threads);
  for (int i=0; i<num_threads; i++) {
    int errnum = pthread_create(&tid[i], NULL, _mul, (void *)&tas[i]);
    if (errnum) {
      show_pthread_error("pthread_create", errnum);
    }
  }

  for (int i=0; i<num_threads; i++) {
    int errnum = pthread_join(tid[i], NULL);
    if (errnum) {
      show_pthread_error("pthread_join", errnum);
    }
  }

  free(tas);
  free(tid);
}


int matrix_invert(gf *A, gf k)
{
  // augment with identity matrix and run Gauss-Jordan elimination
  gf *A_copy = safe_talloc(gf, k * 2*k);
  memset(A_copy, 0, k * 2*k);
  for (gf i=0; i<k; i++) {
    memcpy(&A_copy[i * 2*k], &A[i*k], k);
    A_copy[i * 2*k + k + i] = 1;
  }

  if (_gauss_jordan(A_copy, k, 2*k) < k) { return -1; }

  for (gf i=0; i<k; i++) {
    memcpy(&A[i*k], &A_copy[i * 2*k + k], k);
  }

  free(A_copy);
  return 0;
}


gf matrix_rank(gf *A, gf n, gf m)
{
  // run Gaussian elimination on a copy of the input matrix
  gf *A_copy = safe_talloc(gf, n*m);
  memcpy(A_copy, A, n*m);
  int result = _gaussian_elimination(A_copy, n, m);
  free(A_copy);
  return (result <= 0 || result > 255)? 0 : (gf)result;
}


int matrix_next_submatrix(gf *matrix, gf rows, gf cols, gf k,
                          gf *excluded, gf num_excluded,
                          gf *comb, gf *submatrix)
{
  if (!_next_comb(comb, rows, k, excluded, num_excluded)) {
    return 0;
  }
  for (gf i=0; i<k; i++) {
    memcpy(submatrix + i*cols, matrix + comb[i]*cols, cols);
  }
  return 1;
}


/*  ---------------------------------  */
/* | Internal functions from here on | */
/*  ---------------------------------  */
static int _in(gf needle, gf *haystack, gf size) {
  gf i;
  for (i=0; i<size && haystack[i]!=needle; i++);
  return i==size? -1 : i;
}


static int _next_comb(gf *comb, gf n, gf k, gf *ex, gf num_ex)
{
  if (!k) { return 0; }

  // recursively probe for next combination with subsets of comb[]
  if (!_next_comb(comb+1, n, k-1, ex, num_ex)) {
    gf needed = k;  // how many objects larger than comb[0] are needed
                    // to generate the next combination with this subset
    if (ex && num_ex) {
      for (gf i=0; i<num_ex; i++) {
        if (comb[0] < ex[i]) {
          needed++;  // account for excluded objects
        }
      }
    }
    if (comb[0] >= n - needed) { return 0; }

    // generate the next combination
    if (ex && num_ex) {
      while (_in(++comb[0], ex, num_ex) != -1);
    } else {
      comb[0]++;
    }
    for (gf i=1, j=comb[0]+1; i<k; j++) {
      if (_in(j, ex, num_ex) == -1) { comb[i++]=j; }
    }
  }
  return 1;
}


static int _gaussian_elimination(gf *A, gf n, gf m)
{
  // we maintain an invariant: all leading entries in A are 1
  // this seems to be a good approach for our application
  for (gf i=0, *pA=A; i<n; i++, pA+=m) {
    for (gf j=0; j<m; j++) {
      if (pA[j]) {
        if (pA[j] != 1) {
          gf_mul_bytes(&pA[j], m-j, gf_inv(pA[j]), &pA[j]);
        }
        break;
      }
    }
  }

  int rank = m;
  for (int i=0; i<m; i++) {
    int first = -1;  // first row with non-zero entry in i-th column
    for (int j=i*m+i, k=i; k<n; j+=m, k++) {
      if (A[j]) {
        first = k;
        break;
      }
    }
    if (first == -1) {  // nothing in i-th column
      rank--;
      continue;
    }
    if (first != i) {  // 'swap' if leading row starts with too many 0's
      gf *p1 = &A[i*m+i];
      gf *p2 = &A[first*m+i];
      for (gf j=i; j<m; j++) {
        *p1++ ^= *p2++;  // not swap actually, just add a row to leading row
      }
    }

    // remember we maintain the invariant that all leading entries are 1
    for (int j=i+1; j<n; j++) {
      gf inv=0;
      if (A[j*m + i]) {
        A[j*m + i] = 0;
        gf *p1 = &A[j*m + i+1];
        gf *p2 = &A[i*m + i+1];
        for (int k=i+1; k<m; k++, p1++, p2++) {
          *p1 ^= *p2;
          if (!inv) {
            inv = gf_inv(*p1);
          }
        }
        if (inv) {  // not all zeroes
          gf *p1 = &A[j*m + i+1];
          gf_mul_bytes(p1, m-(i+1), inv, p1);
        }
      }
    }
  }

  return rank;
}


static int _gauss_jordan(gf *A, gf n, gf m)
{
  int rank = _gaussian_elimination(A, n, m);

  for (gf i=rank-1; i>0; i--) {
    gf first = i;
    gf *p_current_row = A + i*m + i;
    while (!*p_current_row) {
      p_current_row++;
      first++;
    }
    gf len = m - first;
    gf *p_subbed_row = A + first;
    for (gf j=0; j<i; j++) {
      gf_mulxor_bytes(p_current_row, len, p_subbed_row[0], p_subbed_row);
      p_subbed_row += m;
    }
  }

  return rank;
}


void *_mul(void *args)
{
  _thread_args *ta = (_thread_args*)args;
  gf *A = ta->A;
  gf *B = ta->B;
  gf *C = ta->C;
  gf n = ta->n;
  gf k = ta->k;
  size_t m = ta->m;
  size_t m_sub = ta->m_sub;
  gf *pA=A, *pB=B, *pC=C;
  for (gf i=0; i<n; i++, pB=B, pC+=m) {
    for (gf j=0; j<k; j++, pA++, pB+=m) {
      gf_mulxor_bytes(pB, m_sub, *pA, pC);
    }
  }
  return NULL;
}

