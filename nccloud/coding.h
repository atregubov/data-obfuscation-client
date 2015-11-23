/**
  * @file coding.h
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Declares the Coding class.
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


#ifndef NCCLOUD_CODING_H
#define NCCLOUD_CODING_H

#include <string>
#include <vector>


/** Abstract base class for coding modules. */
class Coding
{
protected:
  /** Read metadata from disk.
   *  @param[in]       path path to metadata without the ".meta" suffix
   *  @param[out] chunksize chunk size read from the metadata */
  virtual void read_metadata(std::string &path, size_t &chunksize);


  /** Write metadata to disk.
   *  @param[in]      path path to metadata without the ".meta" suffix
   *  @param[in] chunksize chunk size to write to the metadata */
  virtual void write_metadata(std::string &path, size_t chunksize);


  /** Read chunks from disk to a single char buffer.
   *  @param[in]           path path to chunks without the ".chunk_" suffix
   *  @param[in]      chunksize size of a chunk
   *  @param[in]  chunk_indices list of chunk indices to read
   *  @param[out]        chunks single char buffer holding chunks,
   *                            following order in chunk_indices */
  virtual void read_chunks(std::string &path, size_t chunksize,
                           std::vector<int> &chunk_indices, char *chunks);


  /** Write chunks to disk from a single char buffer.
   *  @param[in]          path path to chunks without the ".chunk_" suffix
   *  @param[in]     chunksize size of a chunk
   *  @param[in] chunk_indices list of chunk indices to read
   *  @param[in]        chunks single char buffer holding chunks,
   *                           following order in chunk_indices */
  virtual void write_chunks(std::string &path, size_t chunksize,
                            std::vector<int> &chunk_indices, char *chunks);


public:
  /** Return an instance of a coding scheme, based on user's choice.
   *  @param[in] type choice of coding scheme (0, 1 or 2)
   *  @param[in]    k number of nodes required to reconstruct data
   *  @param[in]    n total number of nodes
   *  @param[in]    w field width, w=8 if we use GF(2^8)
   *  @return an instance of an appropriate subclass of Coding */
  static Coding *use_coding(int type, int k, int n, int t, int w);


  /** Encode a file at srcdir/filename into chunks stored under dstdir.
   *  @param[in]   dstdir destination directory where encoded chunks are stored
   *  @param[in]   srcdir source directory of source file to be encoded
   *  @param[in] filename filename of source file to be encoded
   *  @return 0 on success, -1 on failure */
  virtual int encode_file(std::string &dstdir, std::string &srcdir, std::string &filename) = 0;


  /** Reconstruct a file from chunks under srcdir into dst.
   *  @param[in]           dst pathname of reconstructed file
   *  @param[in]        srcdir directory where retrieved chunks used for decoding are stored
   *  @param[in]      filename filename of file to reconstruct
   *  @param[in] chunk_indices Indices of chunks retrieved for use in decoding
   *  @return 0 on success, -1 on failure */
  virtual int decode_file(std::string &dst, std::string &srcdir, std::string &filename,
                          std::vector<int> &chunk_indices) = 0;


  /** Setup repair and inform the user which chunks to retrieve for use in repair.
   *  @param[in]              srcdir directory where retrieved metadata is at
   *  @param[in]            filename filename of file to repair
   *  @param[in]            erasures list of failed nodes
   *  @param[out] chunks_to_retrieve Indices of chunks to be retrieved for use in repair
   *  @return 0 on success, -1 on failure */
  virtual int repair_file_preprocess(std::string &srcdir, std::string &filename,
                                     std::vector<int> &erasures,
                                     std::vector<int> &chunks_to_retrieve) = 0;


  /** Generate new chunks to replace failed chunks.
   *  @param[in]   dstdir destination directory to store the new chunks
   *  @param[in]   srcdir directory where retrieved chunks are at
   *  @param[in] filename filename of file to repair
   *  @return 0 on success, -1 on failure */
  virtual int repair_file(std::string &dstdir, std::string &srcdir, std::string &filename) = 0;


  /** Return the total number of nodes.
   *  @return the total number of nodes on success, -1 on failure */
  virtual int getn(void) = 0;


  /** Return the number of nodes required to reconstruct data.
   *  @return the total number of nodes on success, -1 on failure */
  virtual int getk(void) = 0;


  /** Return ID of the node where a specified chunk resides.
   *  @param[in] index chunk index
   *  @return the corresponding node id on success, -1 on failure */
  virtual int nodeid(int index) = 0;


  /** Return the number of chunks per node.
   *  @return the number of chunks per node on success, -1 on failure */
  virtual int chunks_per_node(void) = 0;


  /** Return the indices of all chunks on a specified node.
   *  @param[in]           node node number
   *  @param[out] chunk_indices vector of chunk indices on node
   *  @return 0 on success, -1 on failure */
  virtual int chunks_on_node(int node, std::vector<int> &chunk_indices) = 0;


  /** Clear all cached parameters other than n, k and w. */
  virtual void reset(void) = 0;
};

#endif  /* NCCLOUD_CODING_H */

