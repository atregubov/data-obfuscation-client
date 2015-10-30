/**
  * @file matrix.h
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Contains prototypes for functions implemented in matrix.c.
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


#ifndef LIBFMSR_MATRIX_H
#define LIBFMSR_MATRIX_H

#include "gf.h"

/*  ---------------------------  */
/* | the core matrix functions | */
/*  ---------------------------  */
/** Matrix multiplication, C = AB; A is n x k matrix and B is k x m matrix. */
void matrix_mul(gf *A, gf *B, gf *C, gf n, gf k, size_t m);


/** matrix_mul() distributed over (num_threads) threads */
void matrix_mul_p(gf *A, gf *B, gf *C, gf n, gf k, size_t m, int num_threads);


/** Inverts a k x k matrix A.
 *  @return 0 on success and -1 if A is singular */
int matrix_invert(gf *A, gf k);


/** Returns rank(A), where A is an n x m matrix.
 *  @return rank(A) on success and 0 for invalid input */
gf matrix_rank(gf *A, gf n, gf m);


/** Sample the next set of rows from matrix[] in the combinations (rows choose k).
 *  We exclude rows in excluded[], and store results in submatrix[].
 *  @return 0 when all combinations are exhausted */
int matrix_next_submatrix(gf *matrix, gf rows, gf cols, gf k,
                          gf *excluded, gf num_excluded,
                          gf *comb, gf *submatrix);


#endif  /* LIBFMSR_MATRIX_H */

