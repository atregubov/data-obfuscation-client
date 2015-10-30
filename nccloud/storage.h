/**
  * @file storage.h
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Declares the Storage class.
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


#ifndef NCCLOUD_STORAGE_H
#define NCCLOUD_STORAGE_H

#include <map>
#include <string>
#include <vector>


/** Abstract base class for storage modules. */
class Storage
{
public:
  /** Return an instance of a storage scheme, based on user's choice.
   *  @param[in] type choice of coding scheme (0 = FMSR code)
   *  @return an instance of an appropriate subclass of Storage */
  static Storage *use_storage(int type);


  /** Initialize the Storage instance based on storage_param.
   *  @param[in] storage_param dictionary of parameters for this repository
   *  @return 0 on success, -1 on failure (e.g., missing parameters) */
  virtual int init(std::map<std::string, std::string> &storage_param) = 0;


  /** Upload a locally-stored chunk to a destination repository.
   *  @param[in]      srcdir directory under which chunk is stored locally
   *  @param[in]    filename name of file being encoded
   *  @param[in] chunk_index chunk index of chunk to be stored
   *  @return 0 on success, -1 on failure (e.g., missing parameters) */
  virtual int store_chunk(std::string &srcdir, std::string &filename,
                          int chunk_index) = 0;


  /** Batched version of store_chunk(). */
  virtual int store_chunks(std::string &srcdir, std::string &filename,
                           std::vector<int> &chunk_indices) = 0;


  /** Upload a locally-stored metadata file to a destination repository.
   *  @param[in]   srcdir directory under which metadata file is stored locally
   *  @param[in] filename name of file being encoded
   *  @return 0 on success, -1 on failure (e.g., missing parameters) */
  virtual int store_metadata(std::string &srcdir, std::string &filename) = 0;


  /** Combined store_metadata() and store_chunks(). */
  virtual int store_metadata_and_chunks(std::string &srcdir, std::string &filename,
                                        std::vector<int> &chunk_indices) = 0;


  /** Download a chunk from a repository to a local destination.
   *  @param[in]      dstdir directory under which chunk is to be stored locally
   *  @param[in]    filename name of file being decoded
   *  @param[in] chunk_index chunk index of chunk to be retrieved
   *  @return 0 on success, -1 on failure (e.g., missing parameters) */
  virtual int get_chunk(std::string &dstdir, std::string &filename,
                        int chunk_index) = 0;


  /** Batched version of get_chunk(). */
  virtual int get_chunks(std::string &dstdir, std::string &filename,
                         std::vector<int> &chunk_indices) = 0;


  /** Download a metadata file from a repository to a local destination.
   *  @param[in]   dstdir directory under which metadata file is to be stored locally
   *  @param[in] filename name of file being decoded
   *  @return 0 on success, -1 on failure (e.g., missing parameters) */
  virtual int get_metadata(std::string &dstdir, std::string &filename) = 0;


  /** Combined get_metadata() and get_chunks(). */
  virtual int get_metadata_and_chunks(std::string &dstdir, std::string &filename,
                                      std::vector<int> &chunk_indices) = 0;


  /** Delete a chunk from a repository.
   *  @param[in]    filename name of file being deleted
   *  @param[in] chunk_index chunk index of chunk to be deleted
   *  @return 0 on success, -1 on failure (e.g., missing parameters) */
  virtual int delete_chunk(std::string &filename, int chunk_index) = 0;


  /** Batched version of delete_chunk(). */
  virtual int delete_chunks(std::string &filename, std::vector<int> &chunk_indices) = 0;


  /** Delete a metadata file from a repository.
   *  @param[in]    filename name of file being deleted
   *  @return 0 on success, -1 on failure (e.g., missing parameters) */
  virtual int delete_metadata(std::string &filename) = 0;


  /** Combined delete_metadata() and delete_chunks(). */
  virtual int delete_metadata_and_chunks(std::string &filename,
                                         std::vector<int> &chunk_indices) = 0;


  /** Print a list of all files stored on the repository.
   *  @return 0 if accessible, -1 if inaccessible */
  virtual int list_files(void) = 0;


  /** Checks accessibility of repository.
   *  @return 0 if accessible, -1 if inaccessible */
  virtual int check_health(void) = 0;
};

#endif  /* NCCLOUD_STORAGE_H */

