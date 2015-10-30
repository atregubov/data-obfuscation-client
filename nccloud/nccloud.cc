/**
  * @file nccloud.cc
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Main entry point for NCCloud (bin/nccloud).
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
  cout << "Usage: " << prog << " [config] [encode|decode|repair|delete]"
                               " (repair node no.) files..." << endl;
  exit(1);
}


int main(int argc, char **argv)
{
  if (argc < 4) { print_usage(argv[0]); }

  int mode = !strncmp(argv[2], "encode", 7)? 0 :        /* 0 = encode */
            (!strncmp(argv[2], "decode", 7)? 1 :        /* 1 = decode */
            (!strncmp(argv[2], "repair", 7)? 2 :        /* 2 = repair */
            (!strncmp(argv[2], "delete", 7)? 3 : 4)));  /* 3 = delete */
  if (mode == 4 || (mode == 2 && argc < 5)) {
    print_usage(argv[0]);
  }

  // read config
  Config config;
  string config_path(argv[1]);
  config.read_config(config_path);

  // init coding based on config
  vector<string> required_coding_param {"type", "k", "n", "w", "tmpdir"};
  for (auto &it : required_coding_param) {
    if (config.coding_param.count(it) != 1) {
      cerr << "[Coding] " << it << " field missing." << endl;
      exit(1);
    }
  }
  int coding_type = atoi(config.coding_param["type"].c_str());
  int k = atoi(config.coding_param["k"].c_str());
  int n = atoi(config.coding_param["n"].c_str());
  int w = atoi(config.coding_param["w"].c_str());
  string tmpdir = config.coding_param["tmpdir"];
  Coding *coding = Coding::use_coding(coding_type, k, n, w);
  cout << "Coding type: " << coding_type << endl;

  // init storages based on config
  vector<Storage *> storages;
  if (config.storages_param.size() < (unsigned int)n ||
      (mode == 2 && config.storages_param.size() < (unsigned int)n+1)) {
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

  // per-file loop of specified operation
  if (mode == 0) {
    // encode mode
    for (int i=3; i<argc; ++i) {
      string path(argv[i]);
      FileOp::instance()->encode_file(path, coding, storages, tmpdir);
    }
  } else if (mode == 1) {
    // decode mode
    for (int i=3; i<argc; ++i) {
      string filename(argv[i]);
      FileOp::instance()->decode_file(filename, coding, storages, tmpdir);
    }
  } else if (mode == 2) {
    // repair mode
    // backup current config
    string backup_config_path = config_path + ".old";
    config.write_config(backup_config_path);

    // download metadata
    int faulty_node = atoi(argv[3]);
    string filename(argv[4]);
    vector<int> healthy_nodes;
    for (int i=0; i<n; ++i) {
      if (i != faulty_node && storages[i]->check_health() == 0) {
        healthy_nodes.push_back(i);
      } else if (i != faulty_node) {
        cerr << "WARNING: node " << i << " may be down." << endl;
      }
    }
    if (healthy_nodes.size() < (unsigned int)k) {
      cerr << "Insufficient healthy nodes." << endl;
      exit(-1);
    }
    if (storages[healthy_nodes[0]]->get_metadata(tmpdir, filename) == -1) {
      cerr << "Failed to download metadata of " << filename
           << " from node " << healthy_nodes[0] << endl;
      exit(-1);
    }

    // try generating parameters for repair
    vector<int> erasures {faulty_node};
    vector<int> chunks_to_retrieve;
    if (coding->repair_file_preprocess(tmpdir, filename, erasures,
                                       chunks_to_retrieve) == -1) {
      cerr << "Failed to repair." << endl;
      exit(-1);
    }

    // update repository info
    swap(storages[faulty_node], storages[n]);
    storages.erase(storages.begin()+n);
    swap(config.storages_param[faulty_node], config.storages_param[n]);
    config.storages_param.erase(config.storages_param.begin()+n);
    config.write_config(config_path);

    for (int i=4; i<argc; ++i) {
      filename.assign(argv[i]);

      if (i>4) {
        // download metadata for latter files
        if (storages[healthy_nodes[0]]->get_metadata(tmpdir, filename) == -1) {
          print_error(stringstream() << "Failed to download metadata of " << filename
                                     << " from node " << healthy_nodes[0] << endl);
          exit(-1);
        }
      }

      // generate the new chunks
      FileOp::instance()->repair_file(filename, coding, storages,
                                      chunks_to_retrieve, faulty_node, tmpdir);
    }
  } else if (mode == 3) {
    // delete mode
    for (int i=3; i<argc; ++i) {
      string filename(argv[i]);
      FileOp::instance()->delete_file(filename, coding, storages);
    }
  }

  FileOp::instance()->wait();

  return 0;
}

