/**
  * @file codings/rs.h
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Declares the RSCode class.
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


#ifndef NCCLOUD_CODINGS_RS_H
#define NCCLOUD_CODINGS_RS_H

#include <vector>
#include <string>

#include "../coding.h"


/** Coding module class for Reed-Solomon code. */
class RSCode: public Coding
{
  int n, k, m, w;
  int *encode_matrix;
  std::vector<int> failed_nodes;
  std::vector<int> retrieved_chunk_indices;

  size_t padded_size(size_t size);
  void pad_data(char *data, size_t data_size);
  size_t unpad_data(char *data, size_t data_size);

public:
  RSCode(int k, int n, int w);
  ~RSCode();
  int encode_file(std::string &dstdir, std::string &srcdir, std::string &filename);
  int decode_file(std::string &dst, std::string &srcdir, std::string &filename,
                  std::vector<int> &chunk_indices);
  int repair_file_preprocess(std::string &srcdir, std::string &filename,
                             std::vector<int> &erasures,
                             std::vector<int> &chunks_to_retrieve);
  int repair_file(std::string &dstdir, std::string &srcdir, std::string &filename);

  int getn(void);
  int getk(void);
  int nodeid(int index);
  int chunks_per_node(void);
  int chunks_on_node(int node, std::vector<int> &chunk_indices);
  void reset(void);
};

#endif  /* NCCLOUD_CODINGS_RS_H */

