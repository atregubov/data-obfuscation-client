/**
  * @file fmsr.c
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Implements core libfmsr functions callable by third-party apps.
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

#include "fmsr.h"
#include "fmsrutil.h"
#include "matrix.h"
#include "misc.h"

#define NUM_CHECKS_THRESHOLD 1000000000  /**< Number of rounds to check in the
                                              two-phase checking during repair
                                              before declaring failure */

#define LAZY_THRESHOLD 512  /**< Number of rounds to try regenerating new
                                 chunks by heuristic coefficient generation
                                 instead of fully random coefficients */

/*  Turn the types of checks to perform during repair on(1) or off(0).
 *  Checking erMDS property alone will guarantee MDS property and probably rMDS
 *  property as well.  You can turn them on and off just to get a feel of how
 *  the occurrence of bad repair is affected by the checks.  By default, it is
 *  recommended to turn only CHECK_ERMDS on.
 *
 *  Restriction: you can't check rMDS property without checking MDS property */
#define CHECK_ERMDS 1  /**< Check erMDS property? 1 (yes) or 0 (no) */
#define CHECK_MDS   0  /**< Check MDS property? 1 (yes) or 0 (no) */
#define CHECK_RMDS  0  /**< Check rMDS property? 1 (yes) or 0 (no);
                            Note: If yes, CHECK_MDS must be 1 as well */

#define NUM_T 7  /**< Number of threads to distribute matrix_mul() on */

/*  ----------------------------------------------------  */
/* | initialization (call first before doing anything!) | */
/*  ----------------------------------------------------  */
void fmsr_init()
{
  gf_init();
}


/*  ---------------------------------------------------------------  */
/* | helper functions (e.g., for memory allocation in application) | */
/*  ---------------------------------------------------------------  */
gf fmsr_nodeid(gf k, gf n, gf index)
{
  return fmsr_chunks_per_node(k, n)? index / fmsr_chunks_per_node(k, n) : 255;
}


gf fmsr_chunks_per_node(gf k, gf n)
{
  return (k=n-2 && n>=4)? 2 : 255;
}


gf fmsr_chunks_on_node(gf k, gf n, gf node, gf *chunk_indices)
{
  if (!fmsr_chunks_per_node(k, n)) {
    return 255;
  }
  for (gf i=0; i<fmsr_chunks_per_node(k, n); i++) {
    chunk_indices[i] = node*fmsr_chunks_per_node(k, n) + i;
  }
  return 0;
}


gf fmsr_nn(gf k, gf n)
{
  return k*(n-k);
}


gf fmsr_nc(gf k, gf n)
{
  return n*(n-k);
}


size_t fmsr_padded_size(gf k, gf n, size_t size)
{
  return (size/fmsr_nn(k, n) + 1) * fmsr_nn(k, n);
}


/*  ----------------  */
/* | core functions | */
/*  ----------------  */
int fmsr_encode(gf k, gf n, gf *data, size_t data_size, int create_new,
                gf *code_chunks, gf *encode_matrix)
{
  if (!fmsr_encode_support(k, n)) { return -1; }

  gf nn = fmsr_nn(k, n);  // number of native chunks
  gf nc = fmsr_nc(k, n);  // number of code chunks
  size_t chunk_size = fmsr_padded_size(k, n, data_size) / nn;

  // multiply encoding matrix with padded data
  if (create_new) {
    fmsr_create_encode_matrix(k, n, encode_matrix);
  }
  fmsr_pad_data(k, n, data, data_size);
  if (NUM_T > 1) {
    matrix_mul_p(encode_matrix, data, code_chunks, nc, nn, chunk_size, NUM_T);
  } else {
    matrix_mul(encode_matrix, data, code_chunks, nc, nn, chunk_size);
  }

  return 0;
}


int fmsr_decode(gf k, gf n, gf *code_chunks, size_t chunk_size,
                gf *chunk_indices, gf num_chunks, gf *encode_matrix,
                gf *decode_matrix, int create_new,
                gf *data, size_t *data_size)
{
  gf nn = fmsr_nn(k, n);  // number of native chunks
  gf nc = fmsr_nc(k, n);  // number of code chunks
  if (num_chunks < nn) { return -1; }

  gf *submatrix;
  if (create_new) {
    // sample encode_matrix and invert
    submatrix = safe_talloc(gf, nn*nn);
    for (gf i=0, *ptr=submatrix; i<nn; i++, ptr+=nn) {
      if (chunk_indices[i] >= nc) { return -1; }
      memcpy(ptr, &encode_matrix[chunk_indices[i]*nn], nn);
    }
    if (matrix_invert(submatrix, nn) == -1) {
      free(submatrix);
      return -1;
    }
  } else {
    // or, use caller's supplied decoding matrix if available
    submatrix = decode_matrix;
    if (submatrix == NULL) { return -1; }
  }

  // multiply decoding matrix with code chunks
  if (NUM_T > 1) {
    matrix_mul_p(submatrix, code_chunks, data, nn, nn, chunk_size, NUM_T);
  } else {
    matrix_mul(submatrix, code_chunks, data, nn, nn, chunk_size);
  }

  // remove padding and update data size
  *data_size = nn * chunk_size;
  *data_size = fmsr_unpad_data(data, *data_size);

  // update caller's decoding matrix if needed
  if (create_new) {
    if (decode_matrix != NULL) {
      memcpy(decode_matrix, submatrix, nn*nn);
    }
    free(submatrix);
  }

  return 0;
}


int fmsr_repair(gf k, gf n, gf *encode_matrix,
                gf *erasures, gf num_erasures, fmsr_repair_hints *hints,
                gf *new_encode_matrix, gf *repair_matrix,
                gf *chunks_to_retrieve, gf *num_chunks_to_retrieve)
{
  if (!fmsr_repair_support(k, n, num_erasures)) { return -1; }

  *num_chunks_to_retrieve = n-1;  // useless for now since its fixed
  gf nn = fmsr_nn(k, n);  // number of native chunks
  gf nc = fmsr_nc(k, n);  // number of code chunks
  gf *encode_submatrix = safe_talloc(gf, (n-1)*nn);  // ECVs for chunks to retrieve

  // determine chunks to retrieve
  // go with hints if available, else default to the zeroth chunk of each node
  // (MUST provide hints after the first repair)
  gf select = 0;  // 0 or 1, we retrieve the (select)-th chunk from each surviving node
  if (hints && hints->last_repaired != 255) {
    select = hints->last_used ^ ((hints->last_repaired == erasures[0])? 0 : 1);
  }
  for (unsigned int i=0, retrieve_index=0; i<n; i++) {
    if (i == erasures[0]) { continue; }
    chunks_to_retrieve[retrieve_index] = i*2 | select;
    memcpy(encode_submatrix + retrieve_index*nn,
           encode_matrix + chunks_to_retrieve[retrieve_index]*nn, nn);
    retrieve_index++;
  }

  // Calculate lambda's
  //
  // Assuming the erasure removed, nodes and chunks re-numbered sequentially,
  // P_{i*2 + select} = \sum_{j/2!=i} {lambda[i*nn + j - (j/2>i?2:0)] * P_j},
  //     where i is a surviving node no., j is a surviving chunk id, k=0 or k=1.
  //
  // Intuitively, lambda contains n-1 vectors.
  // Each vector contains nn coefficients, which tells us how the (select)-th
  // code chunk in a surviving node can be expressed in terms of code chunks
  // from all other surviving nodes.
  int offset = erasures[0]*2*nn;
  gf *survivor_matrix = safe_talloc(gf, (n-1)*2*nn);  // ECVs for all surviving chunks
  memcpy(survivor_matrix, encode_matrix, offset);
  memcpy(survivor_matrix + offset, encode_matrix + (offset+2*nn), nc*nn - (offset+2*nn));
  gf *lambda = safe_talloc(gf, (n-1)*nn);
  fmsr_calculate_lambda(k, n, survivor_matrix, lambda, select);
  free(survivor_matrix);

  // generate repair coefficients and check validity
  int num_checks=0;
  while (num_checks++ < NUM_CHECKS_THRESHOLD) {
    // first, generate the repair matrix
    if (num_checks < LAZY_THRESHOLD) {
      gf random = rand()%255 + 1;  // 1 <= random <= 255
      for (gf i=0; i<2; i++) {
        gf factor = (i+random)%255 + 1;  // 1 <= factor <= 255
        for (gf j=0, coeff=1; j<n-1; j++) {
          repair_matrix[i*(n-1) + j] = coeff;
          coeff = gf_mul(coeff, factor);
        }
      }
    } else {
      for (gf i=0; i<2*(n-1); i++) {
        repair_matrix[i] = rand()%255 + 1;
      }
    }

    // second, check the specified MDS properties of the repair
    if (!CHECK_ERMDS || fmsr_check_ermds(k, n, repair_matrix, lambda, select)) {
      // update encoding matrix only after passing the erMDS check
      memcpy(new_encode_matrix, encode_matrix, nc*nn);
      if (NUM_T > 1) {
        matrix_mul_p(repair_matrix, encode_submatrix, new_encode_matrix + offset,
                     2, n-1, nn, NUM_T);
      } else {
        matrix_mul(repair_matrix, encode_submatrix, new_encode_matrix + offset,
                   2, n-1, nn);
      }
      if (!CHECK_MDS || fmsr_check_mds(k, n, new_encode_matrix)) {
        if (!CHECK_RMDS || fmsr_check_rmds(k, n, new_encode_matrix, erasures, num_erasures)) {
          if (hints) {
            hints->last_repaired = erasures[0];
            hints->last_used = select;
          }
          free(encode_submatrix);
          free(lambda);
          return num_checks;
        }
      }
    }
  }

  free(encode_submatrix);
  free(lambda);
  return 0;
}


void fmsr_regenerate(gf *repair_matrix, gf rows, gf cols,
                     gf *retrieved_chunks, size_t chunk_size,
                     gf *new_code_chunks)
{
  // Simply a matrix multiplication
  if (NUM_T > 1) {
    matrix_mul_p(repair_matrix, retrieved_chunks, new_code_chunks, rows, cols, chunk_size, NUM_T);
  } else {
    matrix_mul(repair_matrix, retrieved_chunks, new_code_chunks, rows, cols, chunk_size);
  }
}

