/**
  * @file codings/replication.cc
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Implements the Replication class.
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


#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <algorithm>

#include "../common.h"
#include "replication.h"

using namespace std;


/*  ----------------  */
/* | Public methods | */
/*  ----------------  */
Replication::Replication(int k, int n, int w): n(n), retrieved_chunk_index(-1)
{
}


Replication::~Replication()
{
  reset();
}


int Replication::encode_file(string &dstdir, string &srcdir, string &filename)
{
  // check input file size and write to metadata
  string src = srcdir + '/' + filename;
  FILE *infile = fopen(src.c_str(), "rb");
  if (infile == NULL) {
    show_file_error("fopen", src.c_str(), NULL);
  }
  fseek(infile, 0, SEEK_END);
  size_t filesize = ftell(infile);
  fclose(infile);
  string dst = dstdir + '/' + filename;
  write_metadata(dst, filesize);

  // replicate file from src to n copies in dstdir
  dst += ".chunk";
  for (int i=0; i<n; ++i) {
    string chunk_dst = dst + to_string(i);
    copy(chunk_dst, src);
  }

  return 0;
}


int Replication::decode_file(string &dst, string &srcdir, string &filename,
                             vector<int> &chunk_indices)
{
  if (chunk_indices.size() < 1) {
    print_error(stringstream() << "Insufficient chunks retrieved." << endl);
    return -1;
  }

  // load chunk size from metadata
  string src = srcdir + '/' + filename;
  size_t chunksize = 0;
  read_metadata(src, chunksize);

  // check downloaded file size
  retrieved_chunk_index = chunk_indices[0];
  src += ".chunk" + to_string(retrieved_chunk_index);
  FILE *infile = fopen(src.c_str(), "rb");
  if (infile == NULL) {
    show_file_error("fopen", src.c_str(), NULL);
  }
  fseek(infile, 0, SEEK_END);
  size_t filesize = ftell(infile);
  fclose(infile);
  if (filesize != chunksize) {
    print_error(stringstream() << "Downloaded file is corrupted." << endl);
    return -1;
  }

  // copy downloaded file to destination
  copy(dst, src);

  return 0;
}


int Replication::repair_file_preprocess(string &srcdir, string &filename,
                                        vector<int> &erasures,
                                        vector<int> &chunks_to_retrieve)
{
  int i=0;
  for ( ; i<n; ++i) {
    if (find(erasures.begin(), erasures.end(), i) == erasures.end()) {
      chunks_to_retrieve.push_back(i);
      retrieved_chunk_index = i;
      break;
    }
  }
  failed_nodes = erasures;
  return i<n? 0 : -1;
}


int Replication::repair_file(string &dstdir, string &srcdir, string &filename)
{
  if (retrieved_chunk_index == -1) {
    return -1;
  }

  // load chunk size from metadata
  string src = srcdir + '/' + filename;
  size_t chunksize = 0;
  read_metadata(src, chunksize);

  // check downloaded file size
  src += ".chunk" + to_string(retrieved_chunk_index);
  FILE *infile = fopen(src.c_str(), "rb");
  if (infile == NULL) {
    show_file_error("fopen", src.c_str(), NULL);
  }
  fseek(infile, 0, SEEK_END);
  size_t filesize = ftell(infile);
  fclose(infile);
  if (filesize != chunksize) {
    print_error(stringstream() << "Downloaded file is corrupted." << endl);
    return -1;
  }

  // replicate retrieved file from src to a copy in dstdir for each failed node
  string dst = dstdir + '/' + filename + ".chunk";
  for (auto i : failed_nodes) {
    string chunk_dst = dst + to_string(i);
    copy(chunk_dst, src);
  }

  return 0;
}


int Replication::getn(void) { return n; }
int Replication::getk(void) { return 1; }
int Replication::nodeid(int index) { return index; }
int Replication::chunks_per_node(void) { return 1; }


int Replication::chunks_on_node(int node, std::vector<int> &chunk_indices)
{
  chunk_indices.push_back(node);
  return (node>=0 && node<n)? 0 : -1;
}


void Replication::reset(void)
{
  retrieved_chunk_index = -1;
  failed_nodes.erase(failed_nodes.begin(), failed_nodes.end());
}


/*  -----------------  */
/* | Private methods | */
/*  -----------------  */
void Replication::copy(string &dst, string &src)
{
  // copy file from src to dst
  FILE *infile = fopen(src.c_str(), "rb");
  if (infile == NULL) {
    show_file_error("fopen", src.c_str(), NULL);
  }
  FILE *outfile = fopen(dst.c_str(), "wb");
  if (outfile == NULL) {
    show_file_error("fopen", dst.c_str(), NULL);
  }

  char buf[4096];
  while (!feof(infile)) {
    size_t bytes_read = fread(buf, 1, 4096, infile);
    if (bytes_read != 4096 && !feof(infile)) {
      show_file_error("fread", src.c_str(), infile);
    }
    if (fwrite(buf, 1, bytes_read, outfile) != bytes_read) {
      show_file_error("fwrite", dst.c_str(), outfile);
    }
  }
  fclose(infile);
  fclose(outfile);
}

