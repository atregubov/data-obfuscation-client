/**
  * @file fmsrutil.c
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Implements internal functions to supplement the core libfmsr
  *        functions in fmsr.c.
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


#include <stdio.h>
#include <string.h>

#include "fmsr.h"
#include "fmsrutil.h"
#include "matrix.h"
#include "misc.h"

/*  --------------------------------  */
/* | internal FMSR helper functions | */
/*  --------------------------------  */
int fmsr_encode_support(gf k, gf n)
{
  return (n-k!=2 || n<4)? 0 : 1;
}


int fmsr_repair_support(gf k, gf n, gf num_erasures)
{
  return (n-k!=2 || n<4 || num_erasures != 1)? 0 : 1;
}


void fmsr_create_encode_matrix(gf k, gf n, gf *encode_matrix)
{
  // some funny matrix (Cauchy matrix)
  gf rows = fmsr_nc(k, n);
  gf cols = fmsr_nn(k, n);
  for (gf i=0; i<rows; i++) {
    for (gf j=0; j<cols; j++) {
      encode_matrix[i*cols+j] = gf_div(1, i^(255-j));
    }
  }
}


void fmsr_pad_data(gf k, gf n, gf *data, size_t data_size)
{
  data[data_size] = 1;
  memset(data+data_size+1, 0, fmsr_padded_size(k, n, data_size)-data_size-1);
}


size_t fmsr_unpad_data(gf *data, size_t data_size)
{
  gf *ptr = &data[data_size-1];
  if (*ptr) { return *ptr==1? data_size-1 : 0; }
  for (size_t i=data_size; i; i--, ptr--) {
    if (*ptr) { return i-1; }
  }
  return 0;
}


/*  ----------------------------------  */
/* | repair-specific helper functions | */
/*  ----------------------------------  */
void fmsr_calculate_lambda(gf k, gf n, gf *survivor_matrix, gf *lambda, gf select)
{
  unsigned int nn = (unsigned int)fmsr_nn(k, n);  // number of native chunks
  gf *encoding_vector = safe_talloc(gf, nn);  // ECV for currently considered chunk
  gf *submatrix = safe_talloc(gf, nn*nn);     // ECVs for all other surviving nodes

  // calculate lambda[] for chunks on each surviving node
  unsigned int offset = 0;
  unsigned int remaining = nn*nn;
  unsigned int lambda_offset = 0;
  unsigned int two_nn = 2*nn;
  for (gf i=0; i<n-1; i++, lambda_offset+=nn, offset+=two_nn, remaining-=two_nn) {
    memcpy(submatrix, survivor_matrix, offset);
    memcpy(encoding_vector, survivor_matrix + offset + select*nn, nn);
    memcpy(submatrix + offset, survivor_matrix + (offset+two_nn), remaining);
    if (matrix_invert(submatrix, nn) == -1) {
      fprintf(stderr, "\n\t\tunknown error in generating lambda's\n");
      exit(-1);
    }
    matrix_mul(encoding_vector, submatrix, lambda + lambda_offset, 1, nn, nn);
  }

  free(submatrix);
  free(encoding_vector);
}


int fmsr_check_ermds(gf k, gf n, gf *gamma, gf *lambda, gf select)
{
  gf nn = fmsr_nn(k, n);  // number of native chunks

  // check the three inequalities
  // (refer to the INFOCOM '13 paper by Hu, Lee and Shum for details)

  gf lim = n-1;
  for (gf i=0; i<lim; i++) {
    for (gf j=i+1; j<lim; j++) {
      gf a = gamma[i];
      gf b = gamma[j];
      gf c = gamma[(n-1) + i];
      gf d = gamma[(n-1) + j];

      // One: gamma[i] * gamma[(n-1) + j] != gamma[j] * gamma[(n-1) + i]
      if (gf_div(a, b) == gf_div(c, d)) {
        return 0;
      }
    }
  }

  int lambda_select = select;  // stores (i*nn + select)
  for (gf i=0; i<lim; i++, lambda_select += nn) {

    int lambda_select_j = lambda_select;  // stores (i*nn + j*2 + select - (j>i? 2 : 0))
    for (gf j=0; j<lim; j++) {
      if (i==j) { continue; }

      // Two: gamma[{0, (n-1)} + j]
      //      + gamma[{0, (n-1)} + i] * lambda[i*nn + j*2 + select - (j>i? 2 : 0)] != 0
      if ( ( gf_mul(gamma[i], lambda[lambda_select_j]) ^ gamma[j]) == 0 ) {
        return 0;
      }
      if ( ( gf_mul(gamma[(n-1) + i], lambda[lambda_select_j]) ^ gamma[(n-1) + j]) == 0 ) {
        return 0;
      }

      int lambda_select_k = lambda_select_j + 2;  // stores (i*nn + k*2 + select - (k>i? 2 : 0))
      for (gf k=j+1; k<lim; k++) {
        if (i==k) { continue; }

        gf a = gf_mul(gamma[i], lambda[lambda_select_j]) ^ gamma[j];
        gf b = gf_mul(gamma[i], lambda[lambda_select_k]) ^ gamma[k];
        gf c = gf_mul(gamma[(n-1) + i], lambda[lambda_select_j]) ^ gamma[(n-1) + j];
        gf d = gf_mul(gamma[(n-1) + i], lambda[lambda_select_k]) ^ gamma[(n-1) + k];

        // Three: (gamma[j] + gamma[i] * lambda[i*nn + j*2 + select - (j>i? 2: 0)])
        //        * (gamma[(n-1) + k] + gamma[(n-1) + i] * lambda[i*nn + j*2 + select - (j>i? 2: 0)])
        //     != (gamma[k] + gamma[i] * lambda[i*nn + k*2 + select - (k>i? 2: 0)])
        //        * (gamma[(n-1) + j] + gamma[(n-1) + i] * lambda[i*nn + k*2 + select - (k>i? 2: 0)])
        if (gf_div(a, b) == gf_div(c, d)) {
          return 0;
        }

        lambda_select_k += 2;  // does not increment when i==k
      }

      lambda_select_j += 2;  // does not increment when i==j
    }
  }

  return 1;
}


int fmsr_check_mds(gf k, gf n, gf *encode_matrix)
{
  gf nn = fmsr_nn(k, n);

  // check rank of all possible submatrices formed from nCk nodes
  gf *submatrix = safe_talloc(gf, nn*nn);
  gf *choices   = safe_talloc(gf, k);
  for (gf i=0; i<k; choices[i]=i, i++);  // initial choices
  if (matrix_rank(encode_matrix, nn, nn) != nn) {
    free(choices);
    free(submatrix);
    return 0;
  }
  while (matrix_next_submatrix(encode_matrix, n, 2*nn, k, NULL, 0,
                               choices, submatrix))
  {
    if (matrix_rank(submatrix, nn, nn) != nn) {
      free(choices);
      free(submatrix);
      return 0;
    }
  }

  free(choices);
  free(submatrix);
  return 1;
}


static int _get_rmds_degree(gf k, gf n, gf *encode_matrix, gf node)
{
  gf nn = fmsr_nn(k, n);
  gf nc = fmsr_nc(k, n);

  gf *submatrix  = safe_talloc(gf, nn*nn);
  gf *choices    = safe_talloc(gf, nn);
  gf excluded[2] = { (node<<1), (node<<1)+1 };
  for (gf i=0, flag=0; i-flag<nn; i++) {  // initial choices, simply the first k nodes
    if (i == (node<<1)) {
      flag = 2;
      i += 2;
    }
    choices[i-flag] = i;
  }

  int degree = 1;  // our initial choice must have full rank since it satisfies MDS property
  while (matrix_next_submatrix(encode_matrix, nc, nn, nn, excluded, 2,
                               choices, submatrix))
  {
    if (matrix_rank(submatrix, nn, nn) == nn) {
      degree++;
    }
  }

  free(choices);
  free(submatrix);
  return degree;
}


int fmsr_check_rmds(gf k, gf n, gf *encode_matrix,
                    gf *nodes_repaired, gf num_nodes_repaired)
{
  gf nc = fmsr_nc(k, n);

  // For each of the n possible node failures,
  // check the rank of all 2(n-1) C 2k combinations of chunks.
  //
  // The degree number is simply the number of combinations with full rank.
  //
  // The threshold deducts combinations that must be linearly dependent
  // (i.e., all chunks involved in the current round of repair).
  int degree_threshold = (nc-2)*(nc-3)/2 - (n-3)*(n-2)/2;
  for (gf i=0; i<n; i++) {
    if (i == *nodes_repaired) { continue; }
    int degree = _get_rmds_degree(k, n, encode_matrix, i);
    if (degree < degree_threshold) { return 0; }
  }

  return 1;
}

