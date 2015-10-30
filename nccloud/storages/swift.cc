/**
  * @file storages/swift.cc
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Implements the SwiftStorage class.
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
#include <functional>
#include <iostream>
#include <map>
#include <string>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../common.h"
#include "swift.h"

using namespace std;


/*  ----------------  */
/* | Public methods | */
/*  ----------------  */
SwiftStorage::SwiftStorage(): container(""), authurl(""),
                              username(""), password(""), cmd("")
{
}


int SwiftStorage::init(map<string,string> &storage_param)
{
  vector<string> required_param {"path", "authurl", "username", "password"};
  for (auto &it : required_param) {
    if (storage_param.count(it) != 1) {
      cerr << "[Storage:SwiftStorage] " << it << " field missing." << endl;
      return -1;
    }
  }
  container = storage_param["path"];
  authurl   = storage_param["authurl"];
  username  = storage_param["username"];
  password  = storage_param["password"];
  sanitize(container);
  sanitize(authurl);
  sanitize(username);
  sanitize(password);
  cmd = "swift -q -A \"" + authurl + "\" -U \"" +
        username + "\" -K \"" + password + "\" ";
  return 0;
}


/*  ------------------------  */
/* | Upload-related methods | */
/*  ------------------------  */
int SwiftStorage::store_chunk(string &srcdir, string &filename, int chunk_index)
{
  string action = "upload";
  string src = filename + ".chunk" + to_string(chunk_index);
  vector<string> args {src};
  return run_cmd(action, args, cmd, srcdir);
}


int SwiftStorage::store_chunks(string &srcdir, string &filename,
                               vector<int> &chunk_indices)
{
  string action = "upload";
  vector<string> args;
  for (auto chunk_index : chunk_indices) {
    string src = filename + ".chunk" + to_string(chunk_index);
    args.push_back(src);
  }
  return run_cmd(action, args, cmd, srcdir);
}


int SwiftStorage::store_metadata(string &srcdir, string &filename)
{
  string action = "upload";
  string src = filename + ".meta";
  vector<string> args {src};
  return run_cmd(action, args, cmd, srcdir);
}


int SwiftStorage::store_metadata_and_chunks(string &srcdir, string &filename,
                                            vector<int> &chunk_indices)
{
  string action = "upload";
  vector<string> args;
  string meta_src = filename + ".meta";
  args.push_back(meta_src);
  for (auto chunk_index : chunk_indices) {
    string src = filename + ".chunk" + to_string(chunk_index);
    args.push_back(src);
  }
  return run_cmd(action, args, cmd, srcdir);
}


/*  --------------------------  */
/* | Download-related methods | */
/*  --------------------------  */
int SwiftStorage::get_chunk(string &dstdir, string &filename, int chunk_index)
{
  string action = "download";
  string src = filename + ".chunk" + to_string(chunk_index);
  vector<string> args {src};
  return run_cmd(action, args, cmd, dstdir);
}


int SwiftStorage::get_chunks(string &dstdir, string &filename,
                             vector<int> &chunk_indices)
{
  string action = "download";
  vector<string> args;
  for (auto chunk_index : chunk_indices) {
    string src = filename + ".chunk" + to_string(chunk_index);
    args.push_back(src);
  }
  return run_cmd(action, args, cmd, dstdir);
}


int SwiftStorage::get_metadata(string &dstdir, string &filename)
{
  string action = "download";
  string src = filename + ".meta";
  vector<string> args {src};
  return run_cmd(action, args, cmd, dstdir);
}


int SwiftStorage::get_metadata_and_chunks(string &dstdir, string &filename,
                                          vector<int> &chunk_indices)
{
  string action = "download";
  vector<string> args;
  string meta_src = filename + ".meta";
  args.push_back(meta_src);
  for (auto chunk_index : chunk_indices) {
    string src = filename + ".chunk" + to_string(chunk_index);
    args.push_back(src);
  }
  return run_cmd(action, args, cmd, dstdir);
}


/*  ------------------------  */
/* | Delete-related methods | */
/*  ------------------------  */
int SwiftStorage::delete_chunk(string &filename, int chunk_index)
{
  string action = "delete";
  string src = filename + ".chunk" + to_string(chunk_index);
  vector<string> args {src};
  return run_cmd(action, args, cmd);
}


int SwiftStorage::delete_chunks(string &filename, vector<int> &chunk_indices)
{
  string action = "delete";
  vector<string> args;
  for (auto chunk_index : chunk_indices) {
    string src = filename + ".chunk" + to_string(chunk_index);
    args.push_back(src);
  }
  return run_cmd(action, args, cmd);
}


int SwiftStorage::delete_metadata(string &filename)
{
  string action = "delete";
  string src = filename + ".meta";
  vector<string> args {src};
  return run_cmd(action, args, cmd);
}


int SwiftStorage::delete_metadata_and_chunks(string &filename,
                                             vector<int> &chunk_indices)
{
  string action = "delete";
  vector<string> args;
  string meta_src = filename + ".meta";
  args.push_back(meta_src);
  for (auto chunk_index : chunk_indices) {
    string src = filename + ".chunk" + to_string(chunk_index);
    args.push_back(src);
  }
  return run_cmd(action, args, cmd);
}


/*  -----------------  */
/* | Utility methods | */
/*  -----------------  */
int SwiftStorage::list_files(void)
{
  if (check_health() == -1) {
    return -1;
  }
  string action = "list";
  vector<string> args;  // empty vector
  return run_cmd(action, args, cmd);
}


int SwiftStorage::check_health(void)
{
  string action = "stat";
  vector<string> args;  // empty vector
  return run_cmd(action, args, cmd, 1);  // discards stdout
}


/*  -----------------  */
/* | Private methods | */
/*  -----------------  */
void SwiftStorage::sanitize(string &token)
{
  // takes care of most input, unless token starts with a hyphen '-'
  // which *might* be treated as flags by the Swift CLI
  for (int i=token.length(); i>=0; --i) {
    if (token[i]=='\\' || token[i]=='"') {
      token.insert(i, 1, '\\');
    }
  }
}


int SwiftStorage::run_cmd(string &action, vector<string> &args,
                          string &cmd, int discard_stdout)
{
  // full command for invoking the swift CLI
  string cur_cmd = "( " + cmd + action + " \"" + container + "\" ";
  for (auto arg : args) {
    sanitize(arg);
    cur_cmd += " \"" + arg + "\" ";
  }
  cur_cmd += ") ";

  // extra part detecting if errors occurred
  //
  // Since the swift CLI exits with 0 even when there are errors, we determine
  // if errors occurred by detecting the presence of output to stderr using a
  // temporary file.
  //
  // Full command when discard_stdout == 0:
  //   ( cmd_for_swift_CLI ) 2> "err_file";
  //   if [ -s "err_file" ]; then
  //     cat "err_file"; rm -f "err_file"; exit 1;
  //   else
  //     rm -f "err_file"; fi
  hash<string> h;
  string err_file = "error_report_" + to_string(h(cur_cmd));
  cur_cmd += (discard_stdout? "> /dev/null 2> \"" : "2> \"") + err_file +
             "\"; if [ -s \"" + err_file + "\" ]; then cat \"" +
             err_file + "\"; rm -f \"" + err_file +
             "\"; exit 1; else rm -f \"" + err_file + "\"; fi";
  int ret = system(cur_cmd.c_str());
  if (ret == -1) {
    show_error("system");
  } else if (!WIFEXITED(ret)) {
    return -1;
  }
  return WEXITSTATUS(ret)? -1 : 0;
}


int SwiftStorage::run_cmd(string &action, vector<string> &args,
                          string &cmd, string dir, int discard_stdout)
{
  sanitize(dir);
  string cur_cmd = "cd \"" + dir + "\" && " + cmd;
  return run_cmd(action, args, cur_cmd, discard_stdout);
}

