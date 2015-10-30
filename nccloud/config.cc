/**
  * @file config.cc
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Implements the Config class.
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
#include <cstring>
#include <iostream>

#include <map>
#include <string>
#include <vector>

#include "common.h"
#include "config.h"

using namespace std;

#define NCCLOUD_CONFIG_LINE_LENGTH 4096


/*  ----------------  */
/* | Public methods | */
/*  ----------------  */
void Config::read_config(string &path)
{
  FILE *infile = fopen(path.c_str(), "rb");
  if (infile == NULL) {
    show_file_error("fopen", path.c_str(), NULL);
  }

  if (!coding_param.empty()) {
    coding_param.erase(coding_param.begin(), coding_param.end());
  }
  if (!storages_param.empty()) {
    storages_param.erase(storages_param.begin(), storages_param.end());
  }

  while (1) {
    char line[NCCLOUD_CONFIG_LINE_LENGTH] = {0};
    read_line(infile, line, path);
    if (feof(infile)) {
      cerr << "Invalid config file." << endl;
      break;
    }
    if (!strncmp(line, "[Coding]", 9)) {
      read_coding_param(infile, path);
      break;
    }
    if (!strncmp(line, "[Storage]", 10)) {
      read_storages_param(infile, path);
      break;
    }
  }
  fclose(infile);
}


void Config::write_config(string &path)
{
  FILE *outfile = fopen(path.c_str(), "wb");
  if (outfile == NULL) {
    show_file_error("fopen", path.c_str(), NULL);
  }
  write_coding_param(outfile, path);
  if (fwrite("\n", 1, 1, outfile) != 1) {
    show_file_error("fwrite", path.c_str(), outfile);
  }
  write_storages_param(outfile, path);
  fclose(outfile);
}


/*  -----------------  */
/* | Private methods | */
/*  -----------------  */
void Config::extract(char *line, string &key, string &value)
{
  // horribly inefficient but not a big deal
  size_t i=0;
  for ( ; i<strlen(line) && line[i] != '='; ++i);
  if (i+1 >= strlen(line)) {
    cerr << "Invalid line in config: " << line << endl;
    exit(1);
  }
  key.assign(line, i);
  value.assign(&line[i+1]);
}


void Config::read_line(FILE *infile, char line[], string &path)
{
  if (fgets(line, NCCLOUD_CONFIG_LINE_LENGTH, infile)==NULL && !feof(infile)) {
    show_file_error("fgets", path.c_str(), infile);
  }
  if (feof(infile)) {
    return;
  }
  if (line[strlen(line)-1] != '\n') {
    cerr << "Line in config too long: " << line << endl;
    exit(1);
  }
  line[strlen(line)-1] = '\0';
}


void Config::read_coding_param(FILE *infile, string &path)
{
  string key, value;
  while (1) {
    char line[NCCLOUD_CONFIG_LINE_LENGTH] = {0};
    read_line(infile, line, path);
    if (feof(infile)) {
      break;
    }
    if (!strncmp(line, "[Storage]", 10)) {
      read_storages_param(infile, path);
      break;
    }
    if (line[0] != '\0') {
      extract(line, key, value);
      coding_param[key] = value;
    }
  }
}


void Config::read_storages_param(FILE *infile, string &path)
{
  map<string,string> storage_param;
  string key, value;
  while (1) {
    char line[NCCLOUD_CONFIG_LINE_LENGTH] = {0};
    read_line(infile, line, path);
    if (feof(infile)) {
      if (!storage_param.empty()) {
        storages_param.push_back(storage_param);
      }
      break;
    }
    if (!strncmp(line, "[Coding]", 9)) {
      if (!storage_param.empty()) {
        storages_param.push_back(storage_param);
      }
      read_coding_param(infile, path);
      break;
    }
    if (line[0] == '\0') {
      if (!storage_param.empty()) {
        storages_param.push_back(storage_param);
        storage_param.erase(storage_param.begin(), storage_param.end());
      }
    } else {
      extract(line, key, value);
      storage_param[key] = value;
    }
  }
}


void Config::write_map(FILE *outfile, map<string,string> &m, string &path)
{
  if (!m.empty()) {
    string line;
    for (auto &it : m) {
      line.assign(it.first + '=');
      line += it.second + '\n';
      if (fwrite(line.c_str(), 1, line.length(), outfile) != line.length()) {
        show_file_error("fwrite", path.c_str(), outfile);
      }
    }
  }
}


void Config::write_coding_param(FILE *outfile, string &path)
{
  if (fwrite("[Coding]\n", 1, 9, outfile) != 9) {
    show_file_error("fwrite", path.c_str(), outfile);
  }
  write_map(outfile, coding_param, path);
}


void Config::write_storages_param(FILE *outfile, string &path)
{
  if (fwrite("[Storage]\n", 1, 10, outfile) != 10) {
    show_file_error("fwrite", path.c_str(), outfile);
  }
  for (auto &it : storages_param) {
    write_map(outfile, it, path);
    if (fwrite("\n", 1, 1, outfile) != 1) {
      show_file_error("fwrite", path.c_str(), outfile);
    }
  }
}

