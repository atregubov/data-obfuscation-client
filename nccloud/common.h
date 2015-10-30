/**
  * @file common.h
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Implements various convenience functions (e.g., error reporting)
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


#ifndef NCCLOUD_COMMON_H
#define NCCLOUD_COMMON_H

#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <iostream>
#include <sstream>

/** Print error details with perror() and exit */
#define show_error(call) do { \
    fprintf(stderr, "%c[1;31;40m", 0x1B); \
    fprintf(stderr, "%s(%d) in %s:: ", __FILE__, __LINE__, __func__); \
    perror(call); \
    fprintf(stderr, "%c[0m", 0x1B); \
    exit(-1); } while (0)

/** Print error details with perror(), close specified file (if any) and exit */
#define show_file_error(call, filename, fp) do { \
    fprintf(stderr, "%c[1;31;40m", 0x1B); \
    fprintf(stderr, "%s(%d) in %s:: ", __FILE__, __LINE__, __func__); \
    perror(call); \
    fprintf(stderr, "\twhen working with file: %s\n", filename); \
    fprintf(stderr, "%c[0m", 0x1B); \
    if (fp) { fclose(fp); } \
    exit(-1); } while (0)


/** cout for use in multi-threaded environments */
inline void print(std::ostream &s)
{
  std::cout << s.rdbuf();
  std::cout.flush();
  s.clear();
}


/** cerr for use in multi-threaded environments */
inline void print_error(std::ostream &s)
{
  std::cerr << s.rdbuf();
  std::cerr.flush();
  s.clear();
}


/** deallocate an array and reallocate if needed */
template <typename T>
void reset_array(T **array, unsigned int size=0)
{
  if (*array != NULL) {
    delete[] *array;
  }
  *array = size? (new T[size]) : NULL;
}


inline void write_file(std::string dst, char *data, size_t size)
{
  FILE *fp = fopen(dst.c_str(), "wb");
  if (fp == NULL) {
    show_file_error("fopen", dst.c_str(), NULL);
  }
  if (fwrite(data, 1, size, fp) != size) {
    show_file_error("fwrite", dst.c_str(), fp);
  }
  fclose(fp);
}

#endif  /* NCCLOUD_COMMON_H */

