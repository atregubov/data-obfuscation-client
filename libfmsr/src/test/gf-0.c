/**
  * @file test/gf-0.c
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Tests GF(256) implementation of libfmsr.
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
#include <sys/time.h>

#include "../gf.h"


int main()
{
  /* try generating the whole field from x */
  printf("[%s] Testing whole field generation ... ", __FILE__);

  struct timeval start, end;
  gettimeofday(&start, NULL);
  gf_init();
  gettimeofday(&end, NULL);
  time_t sec = end.tv_sec - start.tv_sec;
  suseconds_t usec = end.tv_usec - start.tv_usec;
  gf res=1;
  int appeared[256] = {0};
  for (int i=1; i<256; i++) {
    res = gf_mul(res, (gf)2);
    appeared[res]++;
  }
  for (int i=1; i<256; i++) {
    if (appeared[i] != 1) {
      printf("Failed!\n");
      exit(-1);
    }
  }
  printf("OK! (init: %0.9lf s)\n", sec + usec/1000000.0);

  return 0;
}

