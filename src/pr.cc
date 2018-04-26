// Copyright (c) 2015, The Regents of the University of California (Regents)
// See LICENSE.txt for license details

#include <algorithm>
#include <iostream>
#include <vector>

#include "benchmark.h"
#include "builder.h"
#include "command_line.h"
#include "graph.h"
#include "pvector.h"
#include "pvector_mem.h"

/*CMU ADDED STUFF */
//#include <stdio.h>
//#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

/*
GAP Benchmark Suite
Kernel: PageRank (PR)
Author: Scott Beamer

Will return pagerank scores for all vertices once total change < epsilon

This PR implementation uses the traditional iterative approach. This is done
to ease comparisons to other implementations (often use same algorithm), but
it is not necesarily the fastest way to implement it. It does perform the
updates in the pull direction to remove the need for atomics.
*/

//#define TESTSIZE

//#define UNCACHE_MEM_ADDR 0x8FE00000
#define MEMCHANGES
#define UNCACHE_MEM_ADDR 0x0004a000
#define UNCACHE_SIZE 0x4000


using namespace std;

typedef float ScoreT;
const float kDamp = 0.85;

typedef struct mem_bundle {
    void *mem_pointer;
    int mem_dev;
    size_t alloc_mem_size;
} mBund;


mBund init_mem(){
    //mem we are accessing
    const size_t mem_address = UNCACHE_MEM_ADDR;
    const size_t mem_size = UNCACHE_SIZE;

    size_t alloc_mem_size, page_mask, page_size;
    void *mem_pointer, *virt_addr;
    
    //this opens the mem
    int mem_dev = open("/dev/mem", O_RDWR | O_SYNC);

    //this gets us how much we need to allocate
    page_size = sysconf(_SC_PAGESIZE);
    alloc_mem_size = (((mem_size / page_size) + 1) * page_size);
    page_mask = (page_size - 1);

    //this gets us the pointer to physical mem
    mem_pointer = mmap(NULL,
               alloc_mem_size,
               PROT_READ | PROT_WRITE,
               MAP_SHARED,
               mem_dev,
               (mem_address & ~page_mask)
               );
    //test code
   // int *test_point;
    //test_point = (int*)mem_pointer;
    //*test_point = 2;
    //int test;
    //test = *test_point;
    //printf("mem test = %x\n", test);
    mBund bundle = {mem_pointer, mem_dev, alloc_mem_size};
    return bundle;
}


inline void close_mem(mBund bundle){
    //These must be done
    munmap(bundle.mem_pointer, bundle.alloc_mem_size);
    close(bundle.mem_dev);
}


pvector<ScoreT> PageRankPull(const Graph &g, int max_iters,
                             double epsilon = 0) {
  const ScoreT init_score = 1.0f / g.num_nodes();
  const ScoreT base_score = (1.0f - kDamp) / g.num_nodes();
  pvector<ScoreT> scores(g.num_nodes(), init_score);


//Changed stuff
#ifdef MEMCHANGES
  mBund bundle = init_mem();
  void *mem_pointer = bundle.mem_pointer;
  pvectormem<ScoreT> outgoing_contrib(g.num_nodes(), mem_pointer);
#endif


//Original;
#ifndef MEMCHANGES
  pvector<ScoreT> outgoing_contrib(g.num_nodes());
#endif

#ifdef TESTSIZE
  uint32_t test_size = sizeof(outgoing_contrib);
  printf("original size outgoing = %i\n nodes = %i\n", test_size, g.num_nodes());
#endif
  for (int iter=0; iter < max_iters; iter++) {
    double error = 0;
    #pragma omp parallel for
    for (NodeID n=0; n < g.num_nodes(); n++)
      outgoing_contrib[n] = scores[n] / g.out_degree(n);
    #pragma omp parallel for reduction(+ : error) schedule(dynamic, 64)
    for (NodeID u=0; u < g.num_nodes(); u++) {
      ScoreT incoming_total = 0;
      for (NodeID v : g.in_neigh(u))
        incoming_total += outgoing_contrib[v];
      ScoreT old_score = scores[u];
     // printf("CurrentScore = %f\n", old_score);
      scores[u] = base_score + kDamp * incoming_total;
      error += fabs(scores[u] - old_score);
    }
    printf(" %2d    %lf\n", iter, error);
    if (error < epsilon)
      break;
  }
#ifdef TESTSIZE
  test_size = sizeof(outgoing_contrib);
  printf("original size = %i\n", test_size);
#endif

#ifdef MEMCHANGES
  close_mem(bundle);
#endif
printf("point 3\n");
  return scores;
}


void PrintTopScores(const Graph &g, const pvector<ScoreT> &scores) {
  vector<pair<NodeID, ScoreT>> score_pairs(g.num_nodes());
  for (NodeID n=0; n < g.num_nodes(); n++) {
    score_pairs[n] = make_pair(n, scores[n]);
  }
  int k = 5;
  vector<pair<ScoreT, NodeID>> top_k = TopK(score_pairs, k);
  k = min(k, static_cast<int>(top_k.size()));
  for (auto kvp : top_k)
    cout << kvp.second << ":" << kvp.first << endl;
}


// Verifies by asserting a single serial iteration in push direction has
//   error < target_error
bool PRVerifier(const Graph &g, const pvector<ScoreT> &scores,
                        double target_error) {
printf("point8\n");

  const ScoreT base_score = (1.0f - kDamp) / g.num_nodes();
  pvector<ScoreT> incomming_sums(g.num_nodes(), 0);
  double error = 0;
printf("point9\n");

  for (NodeID u : g.vertices()) {
    printf("point6\n");
    ScoreT outgoing_contrib = scores[u] / g.out_degree(u);
printf("point7\n");

    for (NodeID v : g.out_neigh(u))
      incomming_sums[v] += outgoing_contrib;
  }
  printf("point 1\n");
  for (NodeID n : g.vertices()) {
    error += fabs(base_score + kDamp * incomming_sums[n] - scores[n]);
    incomming_sums[n] = 0;
  }
  PrintTime("Total Error", error);
  return error < target_error;
}

int main(int argc, char* argv[]) {
  CLPageRank cli(argc, argv, "pagerank", 1e-4, 20);
  if (!cli.ParseArgs())
    return -1;
  Builder b(cli);
  Graph g = b.MakeGraph();
  auto PRBound = [&cli] (const Graph &g) {
    return PageRankPull(g, cli.max_iters(), cli.tolerance());
  };
  printf("point 2\n");
  auto VerifierBound = [&cli] (const Graph &g, const pvector<ScoreT> &scores) {
    return PRVerifier(g, scores, cli.tolerance());
  };
  BenchmarkKernel(cli, g, PRBound, PrintTopScores, VerifierBound);
  return 0;
}
