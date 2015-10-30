/**
  * @file misc.h
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Contains prototypes for functions implemented in misc.c.
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


#ifndef LIBFMSR_MISC_H
#define LIBFMSR_MISC_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

/** Print error details with perror() and exit */
#define show_error(call) do { \
    fprintf(stderr, "%c[1;31;40m", 0x1B); \
    fprintf(stderr, "%s(%d) in %s:: ", __FILE__, __LINE__, __func__); \
    perror(call); \
    fprintf(stderr, "%c[0m", 0x1B); \
    exit(-1); } while (0)


/** Print pthread_related error details and exit */
#define show_pthread_error(call, errnum) do { \
    fprintf(stderr, "%c[1;31;40m", 0x1B); \
    fprintf(stderr, "%s(%d) in %s:: ", __FILE__, __LINE__, __func__); \
    errno = errnum; \
    perror(call); \
    fprintf(stderr, "%c[0m", 0x1B); \
    exit(-1); } while (0)


/** malloc() with error-handling */
#define safe_talloc(type, num) (type *)safe_malloc(sizeof(type)*(num))
void *safe_malloc(size_t size);


#ifndef LIBFMSR_GF
#define LIBFMSR_GF
typedef unsigned char gf;
#endif
/** print an n x m matrix A for debug use */
void print_matrix(gf *A, gf n, gf m);

#endif /* LIBFMSR_MISC_H */

