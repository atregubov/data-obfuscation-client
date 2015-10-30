/**
  * @file test/gf-1.c
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Tests multi-byte multiplication in GF(256) implementation of libfmsr.
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

#define TEST_SIZE 1048576
#define NUM_ROUNDS 10
static const int LO=TEST_SIZE/3;
static const int HI=2*TEST_SIZE/3;
static const gf CANARY=0xbb;
static gf sbuf[TEST_SIZE];
static gf mbuf[TEST_SIZE];


int main()
{
  /* Test correctness of multi-byte multiplication */
  printf("[%s] Testing multi-byte multiplication ... ", __FILE__);

  srand(0);  // fixes "random" number for testing
  gf_init();
  memset(mbuf, CANARY, TEST_SIZE);

  time_t ssec=0, msec=0;
  suseconds_t susec=0, musec=0;
  for (int round=0; round<NUM_ROUNDS; round++) {
    gf factor=(gf)rand();

    for (int i=LO; i<HI; i++) {
      mbuf[i] = (gf)rand();
    }
    memcpy(&sbuf[LO], &mbuf[LO], HI-LO);

    /* simple byte-by-byte operations */
    struct timeval start, end;
    gettimeofday(&start, NULL);
    for (int i=LO; i<HI; i++) {
      sbuf[i] = gf_mul(sbuf[i], factor);
    }
    gettimeofday(&end, NULL);
    ssec += end.tv_sec - start.tv_sec;
    susec += end.tv_usec - start.tv_usec;

    /* slightly optimized multi-bytes operations */
    gettimeofday(&start, NULL);
    gf_mul_bytes(&mbuf[LO], HI-LO, factor, &mbuf[LO]);
    gettimeofday(&end, NULL);
    msec += end.tv_sec - start.tv_sec;
    musec += end.tv_usec - start.tv_usec;

    /* check correctness */
    cmp_buf(mbuf, sbuf, LO, HI, TEST_SIZE, CANARY);
  }

  printf("OK! (multi-byte: %0.2lf MiB/s; single-byte: %0.2lf MiB/s)\n",
         NUM_ROUNDS*TEST_SIZE / (1048576*(msec + musec/1000000.0)),
         NUM_ROUNDS*TEST_SIZE / (1048576*(ssec + susec/1000000.0)));
  return 0;
}

