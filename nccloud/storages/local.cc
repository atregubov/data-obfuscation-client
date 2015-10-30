/**
  * @file storages/local.cc
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Implements the LocalStorage class.
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
#include <map>
#include <string>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../common.h"
#include "local.h"

using namespace std;


/*  ----------------  */
/* | Public methods | */
/*  ----------------  */
LocalStorage::LocalStorage(): repository_path("")
{
}


int LocalStorage::init(map<string,string> &storage_param)
{
  if (storage_param.count("path") != 1) {
    print_error(stringstream() << "[Storage:LocalStorage] path field missing." << endl);
    return -1;
  } else {
    repository_path = storage_param["path"] + '/';
  }
  return 0;
}


/*  ------------------------  */
/* | Upload-related methods | */
/*  ------------------------  */
int LocalStorage::store_chunk(string &srcdir, string &filename, int chunk_index)
{
  string src = srcdir + '/' + filename + ".chunk" + to_string(chunk_index);
  string chunk_path = repository_path + filename + ".chunk" + to_string(chunk_index);
  copy(chunk_path, src);
  return 0;
}


int LocalStorage::store_chunks(string &srcdir, string &filename,
                               vector<int> &chunk_indices)
{
  for (auto chunk_index : chunk_indices) {
    store_chunk(srcdir, filename, chunk_index);
  }
  return 0;
}


int LocalStorage::store_metadata(string &srcdir, string &filename)
{
  string src = srcdir + '/' + filename + ".meta";
  string meta_path = repository_path + filename + ".meta";
  copy(meta_path, src);
  return 0;
}


int LocalStorage::store_metadata_and_chunks(string &srcdir, string &filename,
                                            vector<int> &chunk_indices)
{
  store_metadata(srcdir, filename);
  store_chunks(srcdir, filename, chunk_indices);
  return 0;
}


/*  --------------------------  */
/* | Download-related methods | */
/*  --------------------------  */
int LocalStorage::get_chunk(string &dstdir, string &filename, int chunk_index)
{
  string dst = dstdir + '/' + filename + ".chunk" + to_string(chunk_index);
  string chunk_path = repository_path + filename + ".chunk" + to_string(chunk_index);
  copy(dst, chunk_path);
  return 0;
}


int LocalStorage::get_chunks(string &dstdir, string &filename,
                             vector<int> &chunk_indices)
{
  for (auto chunk_index : chunk_indices) {
    get_chunk(dstdir, filename, chunk_index);
  }
  return 0;
}


int LocalStorage::get_metadata(string &dstdir, string &filename)
{
  string dst = dstdir + '/' + filename + ".meta";
  string meta_path = repository_path + filename + ".meta";
  copy(dst, meta_path);
  return 0;
}


int LocalStorage::get_metadata_and_chunks(string &dstdir, string &filename,
                                          vector<int> &chunk_indices)
{
  get_metadata(dstdir, filename);
  get_chunks(dstdir, filename, chunk_indices);
  return 0;
}


/*  ------------------------  */
/* | Delete-related methods | */
/*  ------------------------  */
int LocalStorage::delete_chunk(string &filename, int chunk_index)
{
  string chunk_path = repository_path + filename + ".chunk" + to_string(chunk_index);
  if (remove(chunk_path.c_str())) {
    show_file_error("remove", chunk_path.c_str(), NULL);
  }
  return 0;
}


int LocalStorage::delete_chunks(string &filename, vector<int> &chunk_indices)
{
  for (auto chunk_index : chunk_indices) {
    delete_chunk(filename, chunk_index);
  }
  return 0;
}


int LocalStorage::delete_metadata(string &filename)
{
  string meta_path = repository_path + filename + ".meta";
  if (remove(meta_path.c_str())) {
    show_file_error("remove", meta_path.c_str(), NULL);
  }
  return 0;
}


int LocalStorage::delete_metadata_and_chunks(string &filename,
                                             vector<int> &chunk_indices)
{
  delete_metadata(filename);
  delete_chunks(filename, chunk_indices);
  return 0;
}


/*  -----------------  */
/* | Utility methods | */
/*  -----------------  */
int LocalStorage::list_files(void)
{
  if (check_health() == -1) {
    return -1;
  }

  struct dirent *ent;
  DIR *dir = opendir(repository_path.c_str());
  if (dir == NULL) {
    show_file_error("opendir", repository_path.c_str(), NULL);
  }
  while ((ent=readdir(dir)) != NULL) {
    if (strcmp(ent->d_name, ".") && strcmp(ent->d_name, "..")) {
      cout << ent->d_name << endl;
    }
  }
  if (closedir(dir) == -1) {
    show_file_error("closedir", repository_path.c_str(), NULL);
  }

  return 0;
}


int LocalStorage::check_health(void)
{
  struct stat sb;
  return stat(repository_path.c_str(), &sb)==0 && S_ISDIR(sb.st_mode)? 0 : -1;
}


/*  -----------------  */
/* | Private methods | */
/*  -----------------  */
void LocalStorage::copy(string &dst, string &src)
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

