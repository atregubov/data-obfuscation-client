/**
  * @file gf.h
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Contains prototypes for functions implemented in gf.c.
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


#ifndef LIBFMSR_GF_H
#define LIBFMSR_GF_H

#include <stdlib.h>

#ifndef LIBFMSR_GF
#define LIBFMSR_GF
typedef unsigned char gf;
#endif

/*  ------------------------------------------------------------------  */
/* | initialize all lookup tables (call first before doing anything!) | */
/*  ------------------------------------------------------------------  */
void gf_init(void);


/*  --------------------------------  */
/* | single table lookup operations | */
/*  --------------------------------  */
gf gf_mul(gf a, gf b);  // a*b
gf gf_div(gf a, gf b);  // a/b, undefined for a=0 or b=0
gf gf_inv(gf a);        // a^{-1}, undefined for a=0
gf gf_log(gf a);        // log(a), undefined for a=0
gf gf_x(gf a);          // x^a


/*  ---------------------------------------------------------  */
/* | multi-byte operations (faster than per-byte operations) | */
/*  ---------------------------------------------------------  */
/** Multiplies all (len) bytes in a[] by b, store results in c[].
 *  a[] and c[] can be the same location. */
void gf_mul_bytes(gf *a, size_t len, gf b, gf *c);

/** Multiplies all (len) bytes in a[] by b, bitwise xor results to c[].
 *  a[] and c[] can be the same location. */
void gf_mulxor_bytes(gf *a, size_t len, gf b, gf *c);


#endif  /* LIBFMSR_GF_H */

