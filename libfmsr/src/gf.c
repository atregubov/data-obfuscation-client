/**
  * @file gf.c
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Implements GF(256) arithmetics.
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


#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "gf.h"
#include "misc.h"

// one of the primitive polynomials in GF(256)
static const gf _prime = 0x1d;

static gf _mul[256][256]={{0}};
static gf _exp[510];  // _exp[i] = _exp[i+255] for convenience
static gf _log[256];  // _log[0] not used
static gf _inv[256];  // _inv[0] not used

/*  ------------------------------------------------------------------  */
/* | initialize all lookup tables (call first before doing anything!) | */
/*  ------------------------------------------------------------------  */
void gf_init()
{
  // first the exp, log and inverse tables
  _exp[0] = 1;
  for (gf i=1; i<128; i++) {
    _exp[i] = _exp[i-1] << 1;  // remember x (i.e., 2) generates the field
    if (_exp[i-1] & 0x80) { _exp[i] ^= _prime; }
    _log[_exp[i]] = i;
  }
  for (gf i=128; i; i++) {
    _exp[i] = _exp[i-1] << 1;
    if (_exp[i-1] & 0x80) { _exp[i] ^= _prime; }
    _log[_exp[i]] = i;
    _inv[_exp[i]] = _exp[255-i];
    _inv[_exp[255-i]] = _exp[i];
  }
  _log[1] = 0;
  _inv[1] = 1;
  memcpy(_exp+255, _exp, 255);  // no need to mod during multiplication

  // then the full multiplication table
  for (gf i=1, *p1=_mul[1]; i; i++, p1+=256) {
    p1[i] = _exp[(int)_log[i]<<1];
    for (gf j=1, *p2=&_mul[1][i]; j<i; j++, p2+=256) {
      *p2 = p1[j] = _exp[_log[i] + _log[j]];
    }
  }
}


/*  --------------------------------  */
/* | single table lookup operations | */
/*  --------------------------------  */
gf gf_mul(gf a, gf b) { return _mul[a][b];       }
gf gf_div(gf a, gf b) { return _mul[a][_inv[b]]; }
gf gf_inv(gf a)       { return _inv[a];          }
gf gf_log(gf a)       { return _log[a];          }
gf gf_x(gf a)         { return _exp[a];          }


/*  ---------------------------------------------------------  */
/* | multi-byte operations (faster than per-byte operations) | */
/*  ---------------------------------------------------------  */
// support small endian only
#define gf_uint64_mul(ptr_multiplier, ptr_multiplicand) \
     ((uint64_t)ptr_multiplier[ptr_multiplicand[0]])      | \
    (((uint64_t)ptr_multiplier[ptr_multiplicand[1]])<<8)  | \
    (((uint64_t)ptr_multiplier[ptr_multiplicand[2]])<<16) | \
    (((uint64_t)ptr_multiplier[ptr_multiplicand[3]])<<24) | \
    (((uint64_t)ptr_multiplier[ptr_multiplicand[4]])<<32) | \
    (((uint64_t)ptr_multiplier[ptr_multiplicand[5]])<<40) | \
    (((uint64_t)ptr_multiplier[ptr_multiplicand[6]])<<48) | \
    (((uint64_t)ptr_multiplier[ptr_multiplicand[7]])<<56)


void gf_mul_bytes(gf *a, size_t len, gf b, gf *c)
{
  const gf * restrict pa=a;  // still ok if c==a
  const gf * const restrict pb=_mul[b];
  uint64_t *lpc=(uint64_t *)c;
  size_t i, lim = len>=8? len-7 : 0;
  for (i=0; i<lim; pa+=8, lpc++, i+=8) {
    *lpc = gf_uint64_mul(pb, pa);
  }
  for (; i<len; c[i] = pb[a[i]], i++);
}


void gf_mulxor_bytes(gf *a, size_t len, gf b, gf *c)
{
  const gf * restrict pa=a;  // still ok if c==a
  const gf * const restrict pb=_mul[b];
  uint64_t *lpc=(uint64_t *)c;
  size_t i, lim = len>=8? len-7 : 0;
  for (i=0; i<lim; pa+=8, lpc++, i+=8) {
    *lpc ^= gf_uint64_mul(pb, pa);
  }
  for (; i<len; c[i] ^= pb[a[i]], i++);
}

