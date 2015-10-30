/**
  * @file test/matrix-1.c
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Tests matrix inversion in libfmsr.
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
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "common.h"
#include "../gf.h"
#include "../matrix.h"

#define K 20
#define MARGIN 100
#define NUM_ROUNDS 10
static const gf CANARY=0xbb;
static gf A[K*K + 2*MARGIN];
static gf B[K*K];
static gf C[K*K];
static gf I[K*K]={0};


int main()
{
  printf("[%s] Testing matrix inversion ... ", __FILE__);

  srand(0);  // fixes "random" number for testing
  gf_init();
  memset(A, CANARY, K*K + 2*MARGIN);

  time_t sec=0;
  suseconds_t usec=0;

  /* generate an identity matrix */
  for (gf i=0; i<K; i++) { I[i*(K+1)] = 1; }

  /* generate a Vandermonde matrix for the first test case */
  for (gf *ptr=B, i=0; i<K; i++) {
    for (gf j=0, tmp=1; j<K; j++, tmp=gf_mul(tmp, i+1)) {
      ptr[i*K+j] = tmp;
    }
  }

  for (int round=0; round<NUM_ROUNDS; round++) {
    if (round) {
      for (gf *ptr=B, *lim=B+K*K; ptr<lim; ptr++) {
        *ptr = (gf)rand();
      }
    }
    memcpy(&A[MARGIN], B, K*K);

    struct timeval start, end;
    gettimeofday(&start, NULL);
    int invertible = !matrix_invert(A+MARGIN, K);
    gettimeofday(&end, NULL);
    sec += end.tv_sec - start.tv_sec;
    usec += end.tv_usec - start.tv_usec;

    /* check correctness */
    cmp_buf(A, A, MARGIN, MARGIN + K*K, 2*MARGIN + K*K, CANARY);
    if (invertible) {
      matrix_mul(A+MARGIN, B, C, K, K, K);
      if ( memcmp(C, I, K*K) ) {
        printf("Failed! (wrong answer)\n");
        exit(-1);
      }
    }
  }

  printf("OK! (inversion: %0.9lf s)\n", (sec + usec/1000000.0)/NUM_ROUNDS);
  return 0;
}

