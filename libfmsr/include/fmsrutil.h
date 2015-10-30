/**
  * @file fmsrutil.h
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Contains prototypes for functions implemented in fmsrutil.c.
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


#ifndef LIBFMSR_FMSRUTIL_H
#define LIBFMSR_FMSRUTIL_H

#include "gf.h"

/* for functions below returning int,
 * they return 1 for yes/passed and 0 for no/failed */

/*  --------------------------------  */
/* | internal FMSR helper functions | */
/*  --------------------------------  */
int fmsr_encode_support(gf k, gf n);
int fmsr_repair_support(gf k, gf n, gf num_erasures);
void fmsr_create_encode_matrix(gf k, gf n, gf *encode_matrix);
void fmsr_pad_data(gf k, gf n, gf *data, size_t data_size);
size_t fmsr_unpad_data(gf *data, size_t data_size);  // return true size


/*  ----------------------------------  */
/* | repair-specific helper functions | */
/*  ----------------------------------  */
void fmsr_calculate_lambda(gf k, gf n, gf *survivor_matrix, gf *lambda, gf select);
int fmsr_check_ermds(gf k, gf n, gf *gamma, gf *lambda, gf select);
int fmsr_check_mds(gf k, gf n, gf *encode_matrix);
int fmsr_check_rmds(gf k, gf n, gf *encode_matrix,
                    gf *nodes_repaired, gf num_nodes_repaired);

#endif  /* LIBFMSR_FMSRUTIL_H */

