/**
  * @file codings/ofmsr.h
  * @author Alexey Tregubov (tregubov@usc.edu) (FMSRCode class belongs to Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Implements the OFMSRCode class.
  * 
**/

/* ===================================================================
Copyright (c) 2015, Alexey Tregubov
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

  - Neither the name of the University of Southern California nor the
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
=================================================================== 
*/


#ifndef NCCLOUD_CODINGS_OFMSR_H
#define NCCLOUD_CODINGS_OFMSR_H

#include <vector>
#include <string>

#include "../coding.h"

extern "C"
{
#include <fmsr.h>
}

#ifndef LIBFMSR_GF
#define LIBFMSR_GF
typedef unsigned char gf;
#endif


/** Coding module class for functional minimum-storage regenerating (FMSR) code with obfuscation. */
class OFMSRCode: public Coding
{
  gf k, n, nn, nc;  // (n,k)-FMSR code with nn native chunks and nc code chunks
  gf *encode_matrix, *decode_matrix, *repair_matrix;
  gf *gf_retrieved_chunk_indices;  // chunks retrieved during download or repair
  gf *gf_repair_chunk_indices;     // chunks to repair
  fmsr_repair_hints hints;         // info about previous repair for use in the next repair

  void read_metadata(std::string &path, size_t &chunksize);
  void write_metadata(std::string &path, size_t chunksize);

public:
  OFMSRCode(int k, int n, int w);
  ~OFMSRCode();
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

#endif 

