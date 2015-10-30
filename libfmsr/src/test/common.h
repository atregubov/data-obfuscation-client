/**
  * @file test/common.h
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Implements various convenience functions for test programs.
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


#ifndef LIBFMSR_TEST_COMMON_H
#define LIBFMSR_TEST_COMMON_H

#include <stdio.h>
#include <stdlib.h>

#include "../gf.h"

void simple_matrix_mul(gf *A, gf *B, gf *C, gf n, gf k, size_t m)
{
  memset(C, 0, n*m);
  for (gf i=0; i<n; i++) {
    for (size_t j=0; j<m; j++) {
      for (gf c=0; c<k; c++) {
        C[i*m+j] ^= gf_mul(A[i*k+c], B[c*m+j]);
      }
    }
  }
}


void cmp_buf(gf dst[], gf src[], size_t lo, size_t hi, size_t len, gf canary)
{
  for (size_t i=0; i<lo; i++) {
    if (dst[i] != canary) {
      printf("Failed! (underflow)\n");
      exit(-1);
    }
  }
  for (size_t i=lo; i<hi; i++) {
    if (dst[i] != src[i]) {
      printf("Failed! (wrong answer)\n");
      exit(-1);
    }
  }
  for (size_t i=hi; i<len; i++) {
    if (dst[i] != canary) {
      printf("Failed! (overflow)\n");
      exit(-1);
    }
  }
}

#endif  /* LIBFMSR_TEST_COMMON_H */

