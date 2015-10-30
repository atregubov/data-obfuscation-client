/**
  * @file config.h
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Declares the Config class.
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


#ifndef NCCLOUD_CONFIG_H
#define NCCLOUD_CONFIG_H

#include <cstdio>

#include <map>
#include <string>
#include <vector>


/** Class for dealing with config files. */
class Config
{
  void extract(char *line, std::string &key, std::string &value);
  void read_line(FILE *infile, char line[], std::string &path);
  void read_coding_param(FILE *infile, std::string &path);
  void read_storages_param(FILE *infile, std::string &path);
  void write_map(FILE *outfile, std::map<std::string,std::string> &m, std::string &path);
  void write_coding_param(FILE *outfile, std::string &path);
  void write_storages_param(FILE *outfile, std::string &path);

public:
  /** Stores parameters under the [Coding] section.
   *  For example, if there is a line "k=8" under [Coding], we have:
   *    coding_param["k"] = "8"
   *  */
  std::map<std::string,std::string> coding_param;

  /** Stores parameters under the [Storage] section.
   *  Parameters are grouped by repositories, separated by an empty line.
   *  For example, if config is like:
   *    [Storage]
   *    type = 0
   *
   *    type = 1
   *  then we have:
   *    storages_param[0]["type"] = "0"
   *    storages_param[1]["type"] = "1"
   *  */
  std::vector< std::map<std::string,std::string> > storages_param;


  /** Read config from path into coding_param and storages_param. */
  void read_config(std::string &path);


  /** Write config to path using coding_param and storages_param. */
  void write_config(std::string &path);
};

#endif  /* NCCLOUD_CONFIG_H */

