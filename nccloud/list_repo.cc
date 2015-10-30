/**
  * @file list_repo.cc
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Main entry point for repository listing program (bin/list_repo).
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


#include <cstdlib>
#include <cstring>
#include <iostream>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "coding.h"
#include "common.h"
#include "config.h"
#include "fileop.h"
#include "storage.h"

using namespace std;


void print_usage(char *prog)
{
  cout << "Usage: " << prog << " [config]" << endl;
  exit(1);
}


int main(int argc, char **argv)
{
  if (argc < 2) { print_usage(argv[0]); }

  // read config
  Config config;
  string config_path(argv[1]);
  config.read_config(config_path);
  int n = atoi(config.coding_param["n"].c_str());

  // init storages based on config
  vector<Storage *> storages;
  if (config.storages_param.size() < (unsigned int)n) {
    cerr << "Insufficient repositories provided." << endl;
    exit(1);
  }
  for (auto &it : config.storages_param) {
    if (it.count("type") != 1) {
      cerr << "[Storage] type field missing." << endl;
      exit(1);
    }
    storages.push_back(Storage::use_storage(atoi(it["type"].c_str())));
    if (storages[storages.size()-1]->init(it) == -1) {
      cerr << "[Storage] missing field(s) for repository #" << storages.size()-1 << endl;
      exit(1);
    }
  }

  // list files on all repositories
  for (int i=0; i<n; i++) {
    cout << "On node " << i << ":" << endl;
    if (storages[i]->list_files() == -1) {
      cerr << "Unable to reach node " << i << endl;
    }
    cout << endl;
  }

  return 0;
}

