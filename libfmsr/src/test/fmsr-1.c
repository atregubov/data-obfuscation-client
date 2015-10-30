/**
  * @file test/fmsr-1.c
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Tests single-round file repair with libfmsr.
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
#include "../fmsr.h"
#include "../misc.h"

#define MIN_N 4
#define MAX_N 12
#define FILE_SIZE 10240


int main()
{
  printf("[%s] Testing file repair (once) ...\n", __FILE__);

  srand(0);  // fixes "random" number for testing
  fmsr_init();

  time_t gsec=0, rsec=0;
  suseconds_t gusec=0, rusec=0;

  /* generate "random" file */
  gf nn = fmsr_nn(MAX_N-2, MAX_N);
  gf nc = fmsr_nc(MAX_N-2, MAX_N);
  size_t padded_size = FILE_SIZE + nn;

  gf *data  = safe_talloc(gf, padded_size);
  gf *input = safe_talloc(gf, padded_size);
  gf *code_chunks       = safe_talloc(gf,        nc * padded_size);
  gf *retrieved_chunks  = safe_talloc(gf, (MAX_N-1) * padded_size);
  gf *new_code_chunks   = safe_talloc(gf,         2 * padded_size);
  gf *encode_matrix     = safe_talloc(gf,        nc * nn);
  gf *new_encode_matrix = safe_talloc(gf,        nc * nn);
  gf *repair_matrix     = safe_talloc(gf,         2 * (MAX_N-1));
  gf *chunk_indices     = safe_talloc(gf,        nn);
  gf *chunk_selected    = safe_talloc(gf,        nc);

  for (gf *ptr=data, *lim=ptr+FILE_SIZE; ptr<lim; *ptr++ = (gf)rand());
  memcpy(input, data, FILE_SIZE);

  for (gf n=MIN_N; n<=MAX_N; n++) {
    printf("\t n=%u: ", n);
    memcpy(input, data, FILE_SIZE);
    nn = fmsr_nn(n-2, n);
    nc = fmsr_nc(n-2, n);
    size_t chunk_size = fmsr_padded_size(n-2, n, FILE_SIZE) / nn;

    /* encode */
    int result = fmsr_encode(n-2, n, input, FILE_SIZE, 1, code_chunks, encode_matrix);
    if (result == -1) {
      printf("Failed! (encode failure)\n");
      exit(-1);
    }

    /* choose an erasure and repair */
    gf erasure = (gf)rand() % n;
    gf num_chunks_to_retrieve;
    gf chunks_to_retrieve[MAX_N-1];
    struct timeval start, end;
    gettimeofday(&start, NULL);
    result = fmsr_repair(n-2, n, encode_matrix, &erasure, 1, NULL,
                         new_encode_matrix, repair_matrix,
                         chunks_to_retrieve, &num_chunks_to_retrieve);
    gettimeofday(&end, NULL);
    gsec = end.tv_sec - start.tv_sec;
    gusec = end.tv_usec - start.tv_usec;

    if (result <= 0 || num_chunks_to_retrieve != n-1) {
      printf("Failed! (could not regenerate chunks)\n");
      exit(-1);
    }

    for (gf i=0; i<num_chunks_to_retrieve; i++) {
      memcpy(retrieved_chunks + i*chunk_size,
             code_chunks + chunks_to_retrieve[i]*chunk_size, chunk_size);
    }

    gettimeofday(&start, NULL);
    fmsr_regenerate(repair_matrix, 2, n-1,
                    retrieved_chunks, chunk_size, new_code_chunks);
    gettimeofday(&end, NULL);
    rsec = end.tv_sec - start.tv_sec;
    rusec = end.tv_usec - start.tv_usec;

    memcpy(code_chunks + erasure*2*chunk_size, new_code_chunks, 2*chunk_size);
    memcpy(encode_matrix, new_encode_matrix, nc*nn);

    /* choose chunks to decode from */
    memset(chunk_selected, 0, nc);
    gf selected = 0;
    while (selected < n-2) {
      gf choice = rand() % n;
      if (!chunk_selected[2*choice]) {
        chunk_selected[2*choice] = chunk_selected[2*choice+1] = 1;
        selected++;
      }
    }
    for (gf i=0, index=0; i<nc; i++) {
      if (chunk_selected[i]) {
        chunk_indices[index++] = i;
      }
    }
    for (gf i=0; i<nn; i++) {
      if (chunk_indices[i] > i) {
        memcpy(code_chunks + chunk_size*i,
               code_chunks + chunk_size*chunk_indices[i],
               chunk_size);
      }
    }

    /* decode */
    size_t decoded_file_size = 0;
    result = fmsr_decode(n-2, n, code_chunks, chunk_size,
                         chunk_indices, nn, encode_matrix,
                         NULL, 1,
                         input, &decoded_file_size);

    /* compare */
    if (result == -1) {
      printf("Failed! (wrong encoding matrix)\n");
      exit(-1);
    }
    if (decoded_file_size != FILE_SIZE) {
      printf("Failed! (wrong file size)\n");
      exit(-1);
    }
    if (memcmp(data, input, FILE_SIZE)) {
      printf("Failed! (wrong file content)\n");
      exit(-1);
    }

    printf("(coefficients generation: %0.6lf s; encode: %0.6lf s)\n",
           gsec + gusec/1000000.0,
           rsec + rusec/1000000.0);
  }

  // just for testing
  free(data); free(input); free(code_chunks);
  free(encode_matrix); free(chunk_indices); free(chunk_selected);
  free(new_encode_matrix); free(repair_matrix);

  return 0;
}

