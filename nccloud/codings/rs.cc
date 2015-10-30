/**
  * @file codings/rs.cc
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Implements the RSCode class.
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


#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "../common.h"
#include "rs.h"

extern "C"
{
#include <jerasure.h>
#include <reed_sol.h>
}

using namespace std;


/*  ----------------  */
/* | Public methods | */
/*  ----------------  */
RSCode::RSCode(int k, int n, int w): n(n), k(k), m(n-k), w(w), encode_matrix(NULL)
{
}


RSCode::~RSCode()
{
  reset();
}


int RSCode::encode_file(string &dstdir, string &srcdir, string &filename)
{
  // read input file as aggregated data chunks
  string src = srcdir + '/' + filename;
  FILE *infile = fopen(src.c_str(), "rb");
  if (infile == NULL) {
    show_file_error("fopen", src.c_str(), NULL);
  }
  fseek(infile, 0, SEEK_END);
  size_t filesize = ftell(infile);
  size_t padded_filesize = padded_size(filesize);
  size_t chunksize = padded_filesize / k;
  char *chunks = new char[n * chunksize];
  rewind(infile);
  if (fread(chunks, 1, filesize, infile) != filesize) {
    show_file_error("fread", src.c_str(), infile);
  }
  fclose(infile);

  // pad file and split into data chunks for encoding
  pad_data(chunks, filesize);
  char **data_ptrs = new char*[k];
  for (int i=0; i<k; ++i) {
    data_ptrs[i] = chunks + i*chunksize;
  }

  // encode data chunks to code chunks
  char **code_ptrs = new char*[m];
  for (int i=k; i<n; ++i) {
    code_ptrs[i-k] = chunks + i*chunksize;
  }
  if (encode_matrix == NULL) {
    encode_matrix = new int[k*m];
    int *matrix = reed_sol_vandermonde_coding_matrix(k, m, w);
    memcpy(encode_matrix, matrix, k*m*sizeof(int));
    free(matrix);
  }
  jerasure_matrix_encode(k, m, w, encode_matrix, data_ptrs, code_ptrs, chunksize);
  delete[] data_ptrs;
  delete[] code_ptrs;

  // write chunk size to metadata file
  string dst = dstdir + '/' + filename;
  write_metadata(dst, chunksize);

  // write chunks to files
  vector<int> chunk_indices(n);
  for (int i=0; i<n; ++i) {
    chunk_indices[i] = i;
  }
  write_chunks(dst, chunksize, chunk_indices, chunks);
  delete[] chunks;

  return 0;
}


int RSCode::decode_file(string &dst, string &srcdir, string &filename,
                        vector<int> &chunk_indices)
{
  if (chunk_indices.size() < (unsigned int)k) {
    print_error(stringstream() << "Insufficient chunks retrieved." << endl);
    return -1;
  }

  // load chunk size from metadata
  string src = srcdir + '/' + filename;
  size_t chunksize = 0;
  read_metadata(src, chunksize);
  if (encode_matrix == NULL) {
    encode_matrix = new int[k*m];
    int *matrix = reed_sol_vandermonde_coding_matrix(k, m, w);
    memcpy(encode_matrix, matrix, k*m*sizeof(int));
    free(matrix);
  }

  // load downloaded chunks
  char *chunks = new char[chunk_indices.size() * chunksize];
  read_chunks(src, chunksize, chunk_indices, chunks);

  // categorize into data or code chunks
  char *data_chunks = new char[k * chunksize];
  char **data_ptrs  = new char*[k];
  char *code_chunks = new char[m * chunksize];
  char **code_ptrs  = new char*[m];
  for (int i=0; i<k; ++i) {
    data_ptrs[i] = data_chunks + i*chunksize;
  }
  for (int i=0; i<m; ++i) {
    code_ptrs[i] = code_chunks + i*chunksize;
  }
  char *ptr = chunks;
  for (auto index : chunk_indices) {
    memcpy( (index < k? data_ptrs[index] : code_ptrs[index-k]), ptr, chunksize);
    ptr += chunksize;
  }
  delete[] chunks;

  // treat undownloaded chunks as erasures for convenience
  int *erasures = new int[n-chunk_indices.size()+1];
  for (int i=0, j=0; i<n; ++i) {
    if (find(chunk_indices.begin(), chunk_indices.end(), i) == chunk_indices.end()) {
      erasures[j++] = i;
    }
  }
  erasures[n-chunk_indices.size()] = -1;

  // decode chunks into original data
  if (jerasure_matrix_decode(k, m, w, encode_matrix, 1,
                             erasures, data_ptrs, code_ptrs, chunksize) != 0) {
    return -1;
  }
  size_t decoded_filesize = unpad_data(data_chunks, k*chunksize);
  delete[] erasures;
  delete[] data_ptrs;
  delete[] code_chunks;
  delete[] code_ptrs;

  // write decoded file to dst
  write_file(dst, data_chunks, decoded_filesize);
  delete[] data_chunks;

  return 0;
}


int RSCode::repair_file_preprocess(string &srcdir, string &filename,
                                   vector<int> &erasures,
                                   vector<int> &chunks_to_retrieve)
{
  if (erasures.size() > (unsigned int)m) {
    print_error(stringstream() << "Too many erasures." << endl);
    return -1;
  }
  retrieved_chunk_indices.erase(retrieved_chunk_indices.begin(),
                                retrieved_chunk_indices.end());
  failed_nodes.erase(failed_nodes.begin(), failed_nodes.end());
  for (auto e : erasures) {
    failed_nodes.push_back(e);  // stored internally for repair_file()
  }
  for (int i=0, j=0; j<k && i<n; ++i) {
    if (find(erasures.begin(), erasures.end(), i) == erasures.end()) {
      chunks_to_retrieve.push_back(i);       // list to return to caller
      retrieved_chunk_indices.push_back(i);  // internal list
      ++j;
    }
  }
  return 0;
}


int RSCode::repair_file(string &dstdir, string &srcdir, string &filename)
{
  // load chunk size from metadata
  string src = srcdir + '/' + filename;
  size_t chunksize = 0;
  read_metadata(src, chunksize);

  // load downloaded chunks
  char *chunks = new char[retrieved_chunk_indices.size() * chunksize];
  read_chunks(src, chunksize, retrieved_chunk_indices, chunks);

  // create encoding matrix
  if (encode_matrix == NULL) {
    encode_matrix = new int[k*m];
    int *matrix = reed_sol_vandermonde_coding_matrix(k, m, w);
    memcpy(encode_matrix, matrix, k*m*sizeof(int));
    free(matrix);
  }

  // categorize into data or code chunks
  char *data_chunks = new char[k * chunksize];
  char **data_ptrs  = new char*[k];
  char *code_chunks = new char[m * chunksize];
  char **code_ptrs  = new char*[m];
  for (int i=0; i<k; ++i) {
    data_ptrs[i] = data_chunks + i*chunksize;
  }
  for (int i=0; i<m; ++i) {
    code_ptrs[i] = code_chunks + i*chunksize;
  }
  char *ptr = chunks;
  for (auto index : retrieved_chunk_indices) {
    memcpy( (index < k? data_ptrs[index] : code_ptrs[index-k]), ptr, chunksize);
    ptr += chunksize;
  }
  delete[] chunks;

  // determine whether encoding / decoding is needed
  int need_encode=0, need_decode=0;
  for (auto index : failed_nodes) {
    if (index < k) {
      need_decode = 1;
    } else {
      need_encode = 1;
    }
  }

  // decode to get data chunks
  if (need_decode) {
    // treat undownloaded chunks as erasures for convenience
    int *erasures = new int[n-retrieved_chunk_indices.size()+1];
    for (int i=0, j=0; i<n; ++i) {
      if (find(retrieved_chunk_indices.begin(), retrieved_chunk_indices.end(), i)
          == retrieved_chunk_indices.end()) {
        erasures[j++] = i;
      }
    }
    erasures[n-retrieved_chunk_indices.size()] = -1;

    // decode chunks into original data chunks
    if (jerasure_matrix_decode(k, m, w, encode_matrix, 1,
                               erasures, data_ptrs, code_ptrs, chunksize) != 0) {
      return -1;
    }
    delete[] erasures;
  }

  // encode to get code chunks
  if (need_encode) {
    jerasure_matrix_encode(k, m, w, encode_matrix, data_ptrs, code_ptrs, chunksize);
  }

  // write repaired chunks to disk
  chunks = new char[failed_nodes.size() * chunksize];
  int ctr = 0;
  for (auto index : failed_nodes) {
    memcpy(chunks + (ctr++)*chunksize,
           (index < k? data_ptrs[index] : code_ptrs[index-k]), chunksize);
  }
  string dst = dstdir + '/' + filename;
  write_chunks(dst, chunksize, failed_nodes, chunks);

  delete[] chunks;
  delete[] data_chunks;
  delete[] data_ptrs;
  delete[] code_chunks;
  delete[] code_ptrs;
  return 0;
}


int RSCode::getn(void) { return n; }
int RSCode::getk(void) { return k; }
int RSCode::nodeid(int index) { return index; }
int RSCode::chunks_per_node(void) { return 1; }


int RSCode::chunks_on_node(int node, vector<int> &chunk_indices)
{
  chunk_indices.push_back(node);
  return (node>=0 && node<n)? 0 : -1;
}


void RSCode::reset(void)
{
  reset_array<int>(&encode_matrix);
  failed_nodes.erase(failed_nodes.begin(), failed_nodes.end());
  retrieved_chunk_indices.erase(retrieved_chunk_indices.begin(),
                                retrieved_chunk_indices.end());
}


/*  -----------------  */
/* | Private methods | */
/*  -----------------  */
size_t RSCode::padded_size(size_t size)
{
  return (size/(k*8) + 1) * k*8;
}


void RSCode::pad_data(char *data, size_t data_size)
{
  // pad to a multiple of k*8
  data[data_size] = 1;
  memset(data+data_size+1, 0, padded_size(data_size)-data_size-1);
}


size_t RSCode::unpad_data(char *data, size_t data_size)
{
  // unpad starting from the end of padded data
  char *ptr = &data[data_size-1];
  if (*ptr) { return *ptr==1? data_size-1 : 0; }
  for (size_t i=data_size; i; i--, ptr--) {
    if (*ptr) { return i-1; }
  }
  return 0;
}

