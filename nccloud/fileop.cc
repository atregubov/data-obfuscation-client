/**
  * @file fileop.cc
  * @author Henry Chen (chchen@cse.cuhk.edu.hk)
  * @brief Implements the Job and FileOp classes, and also job pipelining.
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


#include <atomic>
#include <condition_variable>
#include <cstring>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>

#include "fileop.h"

using namespace std;


/*  -------------------------  */
/* | Thread-related routines | */
/*  -------------------------  */
static int num_working_threads = 0;
static queue<Job *> storage_queue;
static queue<Job *> coding_queue;
static condition_variable no_working_threads;
static condition_variable storage_queue_ready;
static condition_variable coding_queue_ready;
static mutex master_mutex;


/** Add a pointer to an object describing a job to the job queue q. */
static void add_job(Job *job, queue<Job *> &q, mutex &m, condition_variable &cv)
{
  m.lock();
  q.push(job);
  m.unlock();
  cv.notify_all();
}


/** Wait until there is a job available for processing.
 *  @param[out] job pointer to a Job object describing the next job
 *  @return 0 if there are no more job (next job is a NULL pointer). */
static int wait_job(Job* &job, queue<Job *> &q, mutex &m, condition_variable &cv)
{
  int end = 0;
  unique_lock<mutex> lock(m);

  if (--num_working_threads == 0) {
    no_working_threads.notify_all();
  }
  while (q.empty()) {
    cv.wait(lock);
  }
  num_working_threads++;

  // doesn't pop when next job is NULL, leave it for the other threads to see
  if ( !(end = (q.front()==NULL)) ) {
    job = q.front();
    q.pop();
  }
  m.unlock();
  return !end;
}


/** Run thread indefinitely, wait for jobs to process
 *  and quit when there will be no more jobs */
static void run_thread(queue<Job *> &q, mutex &m, condition_variable &cv)
{
  m.lock();
  num_working_threads++;
  m.unlock();
  Job *job = NULL;
  while (wait_job(job, q, m, cv)) {
    job->run_job();
    delete job;
  }
}


/*  ---------------------------------  */
/* | ------------------------------- | */
/* ||           Job Class           || */
/* | ------------------------------- | */
/*  ---------------------------------  */

/*  ----------------  */
/* | Public methods | */
/*  ----------------  */
Job::Job(int action, Coding *coding, vector<Storage *> *storages,
         string &tmpdir, string &filename):
             action(action), coding(coding), storages(storages),
             tmpdir(tmpdir), filename(filename), next_job(NULL)
{
}


void Job::follow_job(void)
{
  if (next_job != NULL) {
    if (next_job->action < DIVIDER) {
      add_job(next_job, storage_queue, master_mutex, storage_queue_ready);
    } else {
      add_job(next_job, coding_queue, master_mutex, coding_queue_ready);
    }
  }
}


void Job::run_job(void)
{
  switch (action) {
    case ULMETACHUNKS:
      upload_metadata_and_chunks();
      break;
    case ULMETA:
      upload_metadata();
      break;
    case DLCHUNKS:
      download_chunks();
      break;
    case DLMETA:
      download_metadata();
      break;
    case DECODE:
      decode_file();
      break;
    case REPAIR:
      repair_file();
      break;
    default:
      print(stringstream() << "Invalid job received: " << action << endl);
  }
  follow_job();
}


/*  ------------------------------------  */
/* | Private methods (the job routines) | */
/*  ------------------------------------  */
void Job::upload_metadata_and_chunks(void)
{
  // upload metadata and chunks on a per-node basis
  for (auto nodeid : node_indices) {
    vector<int> cur_chunk_indices;
    for (auto chunk_index : chunk_indices) {
      if (coding->nodeid(chunk_index) == nodeid) {
        cur_chunk_indices.push_back(chunk_index);
      }
    }
    if ((*storages)[nodeid]->
        store_metadata_and_chunks(tmpdir, filename, cur_chunk_indices) == -1) {
      stringstream s;
      s << "Failed to upload " << tmpdir << "/" << filename;
      for (auto cur_chunk_index : cur_chunk_indices) {
        s << " [" << cur_chunk_index << "]";
      }
      s << " to node " << nodeid << endl;
      print_error(s);
      exit(-1);
    }
  }
}


void Job::upload_metadata(void)
{
  // upload metadata to each node
  for (auto nodeid : node_indices) {
    if ((*storages)[nodeid]->store_metadata(tmpdir, filename) == -1) {
      print_error(stringstream() << "Failed to upload metadata of " << filename
                                 << " to node " << nodeid << endl);
      exit(-1);
    }
  }
}


void Job::download_chunks(void)
{
  // download chunks on a per-node basis
  for (auto nodeid : node_indices) {
    vector<int> cur_chunk_indices;
    for (auto chunk_index : chunk_indices) {
      if (coding->nodeid(chunk_index) == nodeid) {
        cur_chunk_indices.push_back(chunk_index);
      }
    }
    if ((*storages)[nodeid]->get_chunks(tmpdir, filename, cur_chunk_indices) == -1) {
      stringstream s;
      s << "Failed to download " << filename;
      for (auto cur_chunk_index : cur_chunk_indices) {
        s << " [" << cur_chunk_index << "]";
      }
      s << " from node " << nodeid << endl;
      print_error(s);
      exit(-1);
    }
  }
}


void Job::download_metadata(void)
{
  // download metadata from the first node
  if ((*storages)[node_indices[0]]->get_metadata(tmpdir, filename) == -1) {
    print_error(stringstream() << "Failed to download metadata of " << filename
                               << " from node " << node_indices[0] << endl);
    exit(-1);
  }
}


void Job::decode_file(void)
{
  string dst = tmpdir + '/' + filename;
  if (coding->decode_file(dst, tmpdir, filename, chunk_indices) == -1) {
    print_error(stringstream() << "Failed to decode: " << filename << endl);
    exit(-1);
  }
}


void Job::repair_file(void)
{
  if (coding->repair_file(tmpdir, tmpdir, filename) == -1) {
    print_error(stringstream() << "Failed to repair"
         << " (check that you have invoked repair_file_preprocess()"
         << " of the corresponding coding scheme): " << filename << endl);
    exit(-1);
  }
}


/*  ------------------------------------  */
/* | ---------------------------------- | */
/* ||           FileOp Class           || */
/* | ---------------------------------- | */
/*  ------------------------------------  */

/*  ----------------  */
/* | Public methods | */
/*  ----------------  */
FileOp *FileOp::instance(void)
{
  static FileOp _instance;
  return &_instance;
}


void FileOp::wait(void)
{
  // Wait until no one is working, which means there should be no more jobs
  unique_lock<mutex> lock(master_mutex);
  while (num_working_threads != 0 || !storage_queue.empty() || !coding_queue.empty()) {
    no_working_threads.wait(lock);
  }
  master_mutex.unlock();

  // NULL pointers indicate to waiting threads there will be no more jobs
  add_job(NULL, storage_queue, master_mutex, storage_queue_ready);
  add_job(NULL, coding_queue, master_mutex, coding_queue_ready);
  for(auto &t : workers) {
    t.join();
  }
}


void FileOp::encode_file(string &path, Coding *coding,
                         vector<Storage *> &storages, string &tmpdir)
{
  print(stringstream() << "Encoding: " << path << endl);

  // encode
  int sep = path.length()-1;
  for ( ; sep>=0 && path[sep]!='/'; --sep);
  string srcdir;
  if (sep >= 0) {
    srcdir.assign(path, 0, sep);
  } else {
    srcdir.assign(".");
  }
  string filename(path, sep+1);
  if (coding->encode_file(tmpdir, srcdir, filename) == -1) {
    print_error(stringstream() << "Failed to encode: "
                               << srcdir << "/" << filename << endl);
    exit(-1);
  }

  // enqueue job: store_metadata_and_chunks()
  Job *job = new Job(Job::ULMETACHUNKS, coding, &storages, tmpdir, filename);
  for (int i=0, j=0; i<coding->getn(); ++i) {
    job->node_indices.push_back(i);
    for (int jj=0; jj<coding->chunks_per_node(); ++jj, ++j) {
      job->chunk_indices.push_back(j);
    }
  }
  add_job(job, storage_queue, master_mutex, storage_queue_ready);
}


void FileOp::decode_file(string &filename, Coding *coding,
                         vector<Storage *> &storages, string &tmpdir)
{
  print(stringstream() << "Decoding: " << filename << endl);

  // look for healthy nodes
  int n = coding->getn();
  vector<int> healthy_nodes;
  for (int i=0; i<n; ++i) {
    if (storages[i]->check_health() == 0) {
      healthy_nodes.push_back(i);
    } else {
      print_error(stringstream() << "WARNING: node " << i << " may be down." << endl);
    }
  }

  unsigned int k = (unsigned int)coding->getk();
  if (healthy_nodes.size() < k) {
    print_error(stringstream() << "Insufficient healthy nodes." << endl);
    exit(-1);
  }

  // download chunks from the first k healthy node, and save their chunk indices
  vector<int> chunk_indices;
  for (unsigned int i=0; i<k; ++i) {
    vector<int> cur_chunk_indices;
    coding->chunks_on_node(healthy_nodes[i], cur_chunk_indices);
    chunk_indices.insert(chunk_indices.end(),
                         cur_chunk_indices.begin(), cur_chunk_indices.end());
  }

  // create job 1: download_chunks()
  Job *job1 = new Job(Job::DLCHUNKS, coding, &storages, tmpdir, filename);
  job1->chunk_indices = chunk_indices;
  for (unsigned int i=0; i<k; ++i) {
    job1->node_indices.push_back(healthy_nodes[i]);
  }

  // create job 2: download_metadata()
  Job *job2 = new Job(Job::DLMETA, coding, &storages, tmpdir, filename);
  job2->node_indices.push_back(healthy_nodes[0]);

  // create job 3: decode_file()
  Job *job3 = new Job(Job::DECODE, coding, &storages, tmpdir, filename);
  job3->chunk_indices = chunk_indices;

  // chain the jobs and enqueue job 1 [download_chunks()]
  job1->next_job = job2;
  job2->next_job = job3;
  add_job(job1, storage_queue, master_mutex, storage_queue_ready);
}


void FileOp::repair_file(string &filename, Coding *coding,
                         vector<Storage *> &storages,
                         vector<int> &chunks_to_retrieve,
                         int faulty_node, string &tmpdir)
{
  print(stringstream() << "Repairing: " << filename << endl);

  // create job 1: download_chunks()
  // (metadata already downloaded during preprocess)
  Job *job1 = new Job(Job::DLCHUNKS, coding, &storages, tmpdir, filename);
  job1->chunk_indices = chunks_to_retrieve;
  bool *node_indices = new bool[coding->getn()]();
  for (auto chunk_index : chunks_to_retrieve) {
    node_indices[coding->nodeid(chunk_index)] = true;
  }
  for (int i=0; i<coding->getn(); ++i) {
    if (node_indices[i] == true) {
      job1->node_indices.push_back(i);
    }
  }
  delete[] node_indices;

  // create job 2: repair_file()
  Job *job2 = new Job(Job::REPAIR, coding, &storages, tmpdir, filename);

  // create job 3: upload_metadata_and_chunks() for new node
  Job *job3 = new Job(Job::ULMETACHUNKS, coding, &storages, tmpdir, filename);
  job3->node_indices.push_back(faulty_node);
  coding->chunks_on_node(faulty_node, job3->chunk_indices);

  // create job 4: upload_metadata() for surviving nodes
  Job *job4 = new Job(Job::ULMETA, coding, &storages, tmpdir, filename);
  for (int i=0, j=0; i<coding->getn(); ++i) {
    if (i == faulty_node) {
      j += coding->chunks_per_node();
      continue;
    }
    job4->node_indices.push_back(i);
    for (int jj=0; jj<coding->chunks_per_node(); ++jj, ++j) {
      job4->chunk_indices.push_back(j);
    }
  }

  // chain the jobs and enqueue job 1 [download_chunks()]
  job1->next_job = job2;
  job2->next_job = job3;
  job3->next_job = job4;
  add_job(job1, storage_queue, master_mutex, storage_queue_ready);
}


void FileOp::delete_file(string &filename, Coding *coding,
                         vector<Storage *> &storages)
{
  print(stringstream() << "Deleting: " << filename << endl);

  int n = coding->getn();
  for (int i=0; i<n; ++i) {
    vector<int> chunk_indices;
    coding->chunks_on_node(i, chunk_indices);
    if (storages[i]->delete_metadata_and_chunks(filename, chunk_indices) == -1) {
      print_error(stringstream() << "Failed to delete " << filename
                                 << " from node " << i << endl);
      exit(-1);
    }
  }
}


/*  -----------------  */
/* | Private methods | */
/*  -----------------  */
FileOp::FileOp()
{
  // spawn one master storage thread and one master coding thread
  // TODO: consider spawning sub-threads within each of the master thread in the future
  workers.push_back(thread(run_thread, ref(storage_queue),
                           ref(master_mutex), ref(storage_queue_ready)));
  workers.push_back(thread(run_thread, ref(coding_queue),
                           ref(master_mutex), ref(coding_queue_ready)));
}

