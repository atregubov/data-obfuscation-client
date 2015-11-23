/**
  * @file codings/ofmsr.cc
  * @author Alexey Tregubov (tregubov@usc.edu) (FMSRCode class belongs to Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Implements the OFMSRCode class.
  * **/

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
=================================================================== */


#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "../common.h"
#include "ofmsr.h"

using namespace std;


/*  ----------------  */
/* | Public methods | */
/*  ----------------  */
OFMSRCode::OFMSRCode(int k, int n, int t, int w):
    k(k), n(n), t(t), nn(fmsr_nn(k, n)), nc(fmsr_nc(k, n)),
    encode_matrix(NULL), decode_matrix(NULL), repair_matrix(NULL),
    gf_retrieved_chunk_indices(NULL), gf_repair_chunk_indices(NULL),
    hints((fmsr_repair_hints){255, 0})
{
  if (w != 8) {
    print_error(stringstream() << "FMSR code only supported for w=8" << endl);
    exit(1);
  }
  fmsr_init();
}


OFMSRCode::~OFMSRCode()
{
  reset();
}


int OFMSRCode::encode_file(string &dstdir, string &srcdir, string &filename)
{
  // read input file as native chunks
  string src = srcdir + '/' + filename;
  FILE *infile = fopen(src.c_str(), "rb");
  if (infile == NULL) {
    show_file_error("fopen", src.c_str(), NULL);
  }
  fseek(infile, 0, SEEK_END);
  size_t filesize = ftell(infile);
  size_t padded_filesize = fmsr_padded_size(k, n, filesize);
  rewind(infile);
  gf *native_chunks = new gf[padded_filesize];
  if (fread(native_chunks, 1, filesize, infile) != filesize) {
    show_file_error("fread", src.c_str(), infile);
  }
  fclose(infile);

  // encode native chunks to code chunks
  size_t chunksize = padded_filesize / nn;
  gf *code_chunks = new gf[nc * chunksize];
  int create_new = encode_matrix? 0 : 1;
  if (create_new) {
    encode_matrix = new gf[nc*nn];
  }
  int result = fmsr_encode(k, n, native_chunks, filesize, create_new,
                           code_chunks, encode_matrix);
  if (result == -1) {
    print_error(stringstream() << "FMSR not supported for k=" << k
                               << " and n=" << n << endl);
    return -1;
  }
  delete[] native_chunks;

  // write encoding matrix, chunk size and default repair hints to metadata file
  string dst = dstdir + '/' + filename;
  write_metadata(dst, chunksize);

  // write code chunks to files
  vector<int> chunk_indices(nc);
  for (unsigned int i=0; i<nc; ++i) {
    chunk_indices[i] = i;
  }
  write_chunks(dst, chunksize, chunk_indices, (char *)code_chunks);
  delete[] code_chunks;

  return 0;
}


int OFMSRCode::decode_file(string &dst, string &srcdir, string &filename,
                          vector<int> &chunk_indices)
{
  unsigned int num_chunks = chunk_indices.size();
  if (num_chunks < nn) {
    print_error(stringstream() << "Insufficient chunks retrieved." << endl);
    return -1;
  }

  // load encoding matrix and chunk size
  string src = srcdir + '/' + filename;
  size_t chunksize = 0;
  read_metadata(src, chunksize);

  // load code chunks
  gf *code_chunks = new gf[nn * chunksize];
  read_chunks(src, chunksize, chunk_indices, (char *)code_chunks);

  // decode code chunks into original data
  reset_array<gf>(&gf_retrieved_chunk_indices, num_chunks);
  for (unsigned int i=0; i<num_chunks; ++i) {
    gf_retrieved_chunk_indices[i] = (gf)chunk_indices[i];
  }
  int create_new = decode_matrix? 0 : 1;
  if (create_new) {
    decode_matrix = new gf[nn*nn];
  }
  gf *decoded_file = new gf[nn * chunksize];
  size_t decoded_filesize = 0;
  int result = fmsr_decode(k, n, code_chunks, chunksize,
                           gf_retrieved_chunk_indices, num_chunks, encode_matrix,
                           decode_matrix, create_new,
                           decoded_file, &decoded_filesize);
  if (result == -1) {
    print_error(stringstream() << "Invalid parameters passed to fmsr_decode()" << endl);
    return -1;
  }
  delete[] code_chunks;

  // write decoded file to dst
  write_file(dst, (char *)decoded_file, decoded_filesize);
  delete[] decoded_file;

  return 0;
}


int OFMSRCode::repair_file_preprocess(string &srcdir, string &filename,
                                     vector<int> &erasures,
                                     vector<int> &chunks_to_retrieve)
{
  if (erasures.size() > 1) {
    print_error(stringstream() << "Too many erasures." << endl);
    if (erasures.size() == 2) {
      print_error(stringstream() << "Try decoding and re-encoding file." << endl);
    }
    return -1;
  }

  // load encoding matrix, chunk size and repair hints
  string src = srcdir + '/' + filename;
  size_t chunksize = 0;
  read_metadata(src, chunksize);

  // determine chunks and repair matrix to use in repair
  // also generate the new encoding matrix and repair hints
  if (chunks_per_node() == -1) {
    return -1;
  }
  gf *gf_erasures = new gf[erasures.size()];
  reset_array<gf>(&gf_repair_chunk_indices, erasures.size() * chunks_per_node());
  for (unsigned int i=0; i<erasures.size(); ++i) {
    gf_erasures[i] = (gf)erasures[i];
    for (int j=0; j<chunks_per_node(); ++j) {
      gf_repair_chunk_indices[i * chunks_per_node() + j] =
          gf_erasures[i] * chunks_per_node() + j;
    }
  }
  gf *new_encode_matrix = new gf[nc*nn];
  reset_array<gf>(&repair_matrix, (n-1) * chunks_per_node());
  reset_array<gf>(&gf_retrieved_chunk_indices, n-1);
  gf num_chunks_to_retrieve = n-1;
  int result = fmsr_repair(k, n, encode_matrix,
                           gf_erasures, erasures.size(), &hints,
                           new_encode_matrix, repair_matrix,
                           gf_retrieved_chunk_indices, &num_chunks_to_retrieve);
  if (result == -1) {
    print_error(stringstream() << "Invalid parameters passed to fmsr_repair()" << endl);
    return -1;
  } else if (result == 0) {
    print_error(stringstream() << "Failed to regenerate.  "
                               << "Try decoding and re-encoding file instead?" << endl);
    return -1;  // failed to find suitable coefficients
  }
  delete[] gf_erasures;

  // inform user of the chunks to download
  for (int i=0; i<num_chunks_to_retrieve; ++i) {
    chunks_to_retrieve.push_back((int)gf_retrieved_chunk_indices[i]);
  }

  // write new encoding matrix and new repair hints to metadata
  memcpy(encode_matrix, new_encode_matrix, nc*nn);
  delete[] new_encode_matrix;
  write_metadata(src, chunksize);

  return 0;
}


int OFMSRCode::repair_file(string &dstdir, string &srcdir, string &filename)
{
  // load chunk size
  string src = srcdir + '/' + filename;
  size_t chunksize = 0;
  read_metadata(src, chunksize);
  write_metadata(src, chunksize); // update encoding matrix and repair hints

  // load retrieved chunks
  gf *retrieved_chunks = new gf[(n-1) * chunksize];
  if (gf_retrieved_chunk_indices == NULL) {
    return -1;  // haven't called repair_file_preprocess()?
  }
  vector<int> chunk_indices(n-1);
  for (int i=0; i<n-1; ++i) {
    chunk_indices[i] = (int)gf_retrieved_chunk_indices[i];
  }
  read_chunks(src, chunksize, chunk_indices, (char *)retrieved_chunks);

  // generate the new chunks
  if (chunks_per_node() == -1) {
    return -1;
  }
  gf *new_code_chunks = new gf[chunks_per_node() * chunksize];
  fmsr_regenerate(repair_matrix, chunks_per_node(), n-1,
                  retrieved_chunks, chunksize,
                  new_code_chunks);
  delete[] retrieved_chunks;

  // write new chunks to dstdir
  if (gf_repair_chunk_indices == NULL) {
    return -1;  // haven't called repair_file_preprocess()?
  }
  vector<int> repair_chunk_indices(chunks_per_node());
  for (int i=0; i<chunks_per_node(); ++i) {
    repair_chunk_indices[i] = gf_repair_chunk_indices[i];
  }
  string dst = dstdir + '/' + filename;
  write_chunks(dst, chunksize, repair_chunk_indices, (char *)new_code_chunks);
  delete[] new_code_chunks;

  return 0;
}


int OFMSRCode::getn(void) { return (int)n; }
int OFMSRCode::getk(void) { return (int)k; }


int OFMSRCode::nodeid(int index)
{
  return (index>255 || index<0)? -1 : (int)(char)fmsr_nodeid(k, n, index);
}


int OFMSRCode::chunks_per_node(void)
{
  return (int)(char)fmsr_chunks_per_node(k, n);
}


int OFMSRCode::chunks_on_node(int node, std::vector<int> &chunk_indices)
{
  int result = chunks_per_node();
  if (result != -1) {
    int chunks_per_node = result;
    gf *gf_chunk_indices = new gf[chunks_per_node];
    result = (int)(char)fmsr_chunks_on_node(k, n, node, gf_chunk_indices);
    if (result != -1) {
      for (int i=0; i<chunks_per_node; ++i) {
        chunk_indices.push_back((int)gf_chunk_indices[i]);
      }
    }
    delete[] gf_chunk_indices;
  }
  return result;
}


void OFMSRCode::reset(void)
{
  reset_array<gf>(&encode_matrix);
  reset_array<gf>(&decode_matrix);
  reset_array<gf>(&repair_matrix);
  reset_array<gf>(&gf_retrieved_chunk_indices);
  reset_array<gf>(&gf_repair_chunk_indices);
}


/*  -----------------  */
/* | Private methods | */
/*  -----------------  */
void OFMSRCode::read_metadata(string &path, size_t &chunksize)
{
  // read encoding matrix, chunk size and repair hints from existing metadata
  string meta_path = path + ".meta";
  FILE *metafile = fopen(meta_path.c_str(), "rb");
  if (metafile == NULL) {
    show_file_error("fopen", meta_path.c_str(), NULL);
  }
  int update = 0;
  if (encode_matrix == NULL) {
    update = 1;
    encode_matrix = new gf[nc*nn];
    if (fread(encode_matrix, 1, nc*nn, metafile) != nc*nn) {
      show_file_error("fread", meta_path.c_str(), metafile);
    }
  }
  fseek(metafile, nc*nn, SEEK_SET);
  char stchunksize[25] = {0};
  if (fread(stchunksize, 1, 24, metafile) <= 0) {
    show_file_error("fread", meta_path.c_str(), metafile);
  }
  int st_len = strlen(stchunksize);
  if (update) {  // prevents overwriting new hints with stale hints
    hints.last_used = stchunksize[st_len-1] - '0';
    stchunksize[st_len-1] = 0;
    hints.last_repaired = strtoul(&stchunksize[st_len-4], NULL, 10);
  }
  stchunksize[st_len-4] = 0;
  chunksize = strtoul(stchunksize, NULL, 10);
  fclose(metafile);
}


void OFMSRCode::write_metadata(string &path, size_t chunksize)
{
  // write encoding matrix, chunk size and repair hints to metadata
  string meta_path = path + ".meta";
  FILE *metafile = fopen(meta_path.c_str(), "wb");
  if (metafile == NULL) {
    show_file_error("fopen", meta_path.c_str(), NULL);
  }
  if (fwrite(encode_matrix, 1, nc*nn, metafile) != nc*nn) {
    show_file_error("fwrite", meta_path.c_str(), metafile);
  }
  string stchunksize = to_string(chunksize);
  if (fwrite(stchunksize.c_str(), 1, stchunksize.length(), metafile)
      != stchunksize.length()) {
    show_file_error("fwrite", meta_path.c_str(), metafile);
  }
  string sthints = to_string((unsigned long)hints.last_repaired*10 + hints.last_used);
  sthints.insert(0, string(4-sthints.length(), '0'));
  if (fwrite(sthints.c_str(), 1, sthints.length(), metafile) != sthints.length()) {
    show_file_error("fwrite", meta_path.c_str(), metafile);
  }
  fclose(metafile);
}

