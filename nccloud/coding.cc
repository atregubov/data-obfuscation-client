/**
  * @file coding.cc
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Implements methods of the Coding class that are not purely virtual.
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


#include "coding.h"
#include "codings/fmsr.h"
#include "codings/ofmsr.h"
#include "codings/replication.h"
#include "codings/rs.h"
#include "common.h"

using namespace std;


Coding *Coding::use_coding(int type, int k, int n, int t, int w)
{
  switch (type) {
    case 0:
      return new FMSRCode(k, n, w);
    case 1:
      return new RSCode(k, n, w);
    case 2:
      return new Replication(k, n, w);
	 case 3:
      return new OFMSRCode(k, n, t, w);
    default:
      return new FMSRCode(k, n, w);
  }
}


void Coding::read_metadata(string &path, size_t &chunksize)
{
  // read chunk size from existing metadata
  string meta_path = path + ".meta";
  FILE *metafile = fopen(meta_path.c_str(), "rb");
  if (metafile == NULL) {
    show_file_error("fopen", meta_path.c_str(), NULL);
  }
  char stchunksize[21] = {0};
  if (fread(stchunksize, 1, 20, metafile) <= 0) {
    show_file_error("fread", meta_path.c_str(), metafile);
  }
  chunksize = strtoul(stchunksize, NULL, 10);
  fclose(metafile);
}


void Coding::write_metadata(string &path, size_t chunksize)
{
  // write chunk size to metadata
  string meta_path = path + ".meta";
  string stchunksize = to_string(chunksize);
  write_file(meta_path, (char *)stchunksize.c_str(), stchunksize.length());
}


void Coding::read_chunks(string &path, size_t chunksize,
                         vector<int> &chunk_indices, char *chunks)
{
  // read chunks stored as path.chunk_ into a single buffer "chunks"
  string chunk_partial_path = path + ".chunk";
  for (unsigned int i=0; i<chunk_indices.size(); ++i) {
    string chunk_path = chunk_partial_path + to_string(chunk_indices[i]);
    FILE *infile = fopen(chunk_path.c_str(), "rb");
    if (infile == NULL) {
      show_file_error("fopen", chunk_path.c_str(), NULL);
    }
    if (fread(&chunks[i*chunksize], 1, chunksize, infile) != chunksize) {
      show_file_error("fread", chunk_path.c_str(), infile);
    }
    fclose(infile);
  }
}


void Coding::write_chunks(string &path, size_t chunksize,
                          vector<int> &chunk_indices, char *chunks)
{
  // write chunks in the buffer "chunks" to their corresponding destinations
  string chunk_partial_path = path + ".chunk";
  for (unsigned int i=0; i<chunk_indices.size(); ++i) {
    string chunk_path = chunk_partial_path + to_string(chunk_indices[i]);
    write_file(chunk_path, chunks + i*chunksize, chunksize);
  }
}

