/**
  * @file fileop.h
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Declares the Job and FileOp classes.
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


#ifndef NCCLOUD_FILEOP_H
#define NCCLOUD_FILEOP_H

#include <string>
#include <thread>
#include <vector>

#include "coding.h"
#include "common.h"
#include "storage.h"


/** Class for a storage/coding job assigned to a
 *  master storage/coding thread in FileOp. */
class Job
{
  /* Storage job routines. */
  void upload_metadata_and_chunks(void);
  void upload_metadata(void);
  void download_chunks(void);
  void download_metadata(void);

  /* Coding job routines. */
  void decode_file(void);
  void repair_file(void);

public:
  /** All types of jobs.  Storage jobs < DIVIDER; coding jobs > DIVIDER. */
  enum ACTIONS { ULMETACHUNKS, ULMETA, DLCHUNKS, DLMETA,
                 DIVIDER, DECODE, REPAIR };

  int action;  /**< job to carry out (see enum ACTIONS) */
  Coding *coding;  /**< coding scheme used */
  std::vector<Storage *> *storages;  /**< all storage nodes used */
  std::string tmpdir;  /**< path to temporary directory */
  std::string filename;  /**< name of file to act on */
  std::vector<int> chunk_indices;  /**< indices of chunks involved in current job */
  std::vector<int> node_indices;   /**< indices of nodes involved in current job */
  Job *next_job;  /**< pointer to an object describing the next job (NULL for none) */


  Job(int action, Coding *coding, std::vector<Storage *> *storages,
      std::string &tmpdir, std::string &filename);


  /** Add the next job (if any) to the appropriate job queue. */
  void follow_job(void);


  /** Run the corresponding job routine for the current job, then call follow_job(). */
  void run_job(void);
};


/** Singleton class for file operations. */
class FileOp
{
  /** Worker threads for processing storage/coding jobs. */
  std::vector<std::thread> workers;

  FileOp();

public:
  /** Returns a singleton instance of FileOp. */
  static FileOp *instance(void);


  /** Wait for all storage/coding jobs to finish. */
  void wait(void);


  /** Encode and upload a file.
   *  @param[in]     path local path of file to upload
   *  @param[in]   coding Coding instance describing the coding scheme used
   *  @param[in] storages Storage instances describing the repositories
   *  @param[in]   tmpdir path to temporary directory */
  void encode_file(std::string &path, Coding *coding,
                   std::vector<Storage *> &storages, std::string &tmpdir);


  /** Download and decode a file.
   *  @param[in] filename name of file to download
   *  @param[in]   coding Coding instance describing the coding scheme used
   *  @param[in] storages Storage instances describing the repositories
   *  @param[in]   tmpdir path to temporary directory */
  void decode_file(std::string &filename, Coding *coding,
                   std::vector<Storage *> &storages, std::string &tmpdir);


  /** Repair a file.
   *  @param[in]           filename name of file to repair
   *  @param[in]             coding Coding instance describing the coding scheme used
   *  @param[in]           storages Storage instances describing the repositories
   *  @param[in] chunks_to_retrieve indices of chunks to be retrieved
   *  @param[in]        faulty_node node to repair
   *  @param[in]             tmpdir path to temporary directory */
  void repair_file(std::string &filename, Coding *coding,
                   std::vector<Storage *> &storages,
                   std::vector<int> &chunks_to_retrieve,
                   int faulty_node, std::string &tmpdir);


  /** Delete a file.
   *  @param[in] filename name of file to delete
   *  @param[in]   coding Coding instance describing the coding scheme used
   *  @param[in] storages Storage instances describing the repositories */
  void delete_file(std::string &filename, Coding *coding,
                   std::vector<Storage *> &storages);
};

#endif  /* NCCLOUD_FILEOP_H */

