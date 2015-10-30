/**
  * @file test/matrix-0.c
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Tests matrix multiplication in libfmsr.
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
#include "../misc.h"

/* C = AB; A is an N x K matrix, B is a K x M matrix */
#define N 8
#define K 4
#define M 1048576
#define MARGIN 100
#define NUM_ROUNDS 1
static const gf CANARY=0xbb;
static gf A_parallel[N*K + 2*MARGIN];
static gf B_parallel[K*M + 2*MARGIN];
static gf C_parallel[N*M + 2*MARGIN];
static gf A_opt[N*K + 2*MARGIN];
static gf B_opt[K*M + 2*MARGIN];
static gf C_opt[N*M + 2*MARGIN];
static gf A_simple[N*K + 2*MARGIN];
static gf B_simple[K*M + 2*MARGIN];
static gf C_simple[N*M + 2*MARGIN];


int main()
{
  printf("[%s] Testing matrix multiplication ... ", __FILE__);

  srand(0);  // fixes "random" number for testing
  gf_init();
  memset(A_parallel, CANARY, N*K + 2*MARGIN);
  memset(B_parallel, CANARY, K*M + 2*MARGIN);
  memset(C_parallel, CANARY, N*M + 2*MARGIN);
  memset(A_opt, CANARY, N*K + 2*MARGIN);
  memset(B_opt, CANARY, K*M + 2*MARGIN);
  memset(C_opt, CANARY, N*M + 2*MARGIN);

  time_t ssec=0, msec=0, psec=0;
  suseconds_t susec=0, musec=0, pusec=0;

  for (int round=0; round<NUM_ROUNDS; round++) {
    for (gf *ptr=A_opt+MARGIN, *lim=ptr+N*K; ptr<lim; *ptr++ = (gf)rand());
    for (gf *ptr=B_opt+MARGIN, *lim=ptr+K*M; ptr<lim; *ptr++ = (gf)rand());
    for (gf *ptr=C_opt+MARGIN, *lim=ptr+N*M; ptr<lim; *ptr++ = (gf)rand());
    memcpy(&A_parallel[MARGIN], &A_opt[MARGIN], N*K);
    memcpy(&B_parallel[MARGIN], &B_opt[MARGIN], K*M);
    memcpy(&C_parallel[MARGIN], &C_opt[MARGIN], N*M);
    memcpy(&A_simple[MARGIN], &A_opt[MARGIN], N*K);
    memcpy(&B_simple[MARGIN], &B_opt[MARGIN], K*M);
    memcpy(&C_simple[MARGIN], &C_opt[MARGIN], N*M);

    /* straightforward matrix multiplication */
    struct timeval start, end;
    gettimeofday(&start, NULL);
    simple_matrix_mul(A_simple+MARGIN, B_simple+MARGIN, C_simple+MARGIN, N, K, M);
    gettimeofday(&end, NULL);
    ssec += end.tv_sec - start.tv_sec;
    susec += end.tv_usec - start.tv_usec;

    /* slightly optimized matrix multiplication */
    gettimeofday(&start, NULL);
    matrix_mul(A_opt+MARGIN, B_opt+MARGIN, C_opt+MARGIN, N, K, M);
    gettimeofday(&end, NULL);
    msec += end.tv_sec - start.tv_sec;
    musec += end.tv_usec - start.tv_usec;

    /* parallelized version */
    gettimeofday(&start, NULL);
    matrix_mul_p(A_parallel+MARGIN, B_parallel+MARGIN, C_parallel+MARGIN, N, K, M, 7);
    gettimeofday(&end, NULL);
    psec += end.tv_sec - start.tv_sec;
    pusec += end.tv_usec - start.tv_usec;

    /* check correctness */
    cmp_buf(A_parallel, A_simple, MARGIN, MARGIN + N*K, 2*MARGIN + N*K, CANARY);
    cmp_buf(B_parallel, B_simple, MARGIN, MARGIN + K*M, 2*MARGIN + K*M, CANARY);
    cmp_buf(C_parallel, C_simple, MARGIN, MARGIN + N*M, 2*MARGIN + N*M, CANARY);
    cmp_buf(A_opt, A_simple, MARGIN, MARGIN + N*K, 2*MARGIN + N*K, CANARY);
    cmp_buf(B_opt, B_simple, MARGIN, MARGIN + K*M, 2*MARGIN + K*M, CANARY);
    cmp_buf(C_opt, C_simple, MARGIN, MARGIN + N*M, 2*MARGIN + N*M, CANARY);
  }

  printf("OK! (parallel: %0.3lf s; optimized: %0.3lf s; naive: %0.3lf s)\n",
         psec + pusec/1000000.0, msec + musec/1000000.0, ssec + susec/1000000.0);
  return 0;
}

