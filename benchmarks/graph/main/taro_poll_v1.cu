#include <taro.hpp>

#include "../executor/taro_poll_v1.hpp"

#include <3rd-party/CLI11/CLI11.hpp>

#include <iostream>

int main(int argc, char* argv[]) {

  CLI::App app{"Graph Benchmark"};

  std::string graph{"ParallelGraph"};
  app.add_option(
    "-g, --graph", 
    graph, 
    "select graph(SerialGraph, ParallelGraph, Tree, RandomDAG, MapReduce, Wavefront), default is ParallelGraph" 
  );

  //std::string job{"loop"};
  //app.add_option(
    //"-j, --job",
    //job,
    //"select job(matmul, cudaflow_reduce, loop), default is loop"
  //);

  size_t cpu_time{2};
  app.add_option(
    "--cpu_overhead", 
    cpu_time, 
    "set CPU overhead for each task (ms)"
  );

  size_t gpu_time{2};
  app.add_option(
    "--gpu_overhead", 
    gpu_time, 
    "set GPU overhead for each task (ms)"
  );

  std::vector<int> args;
  app.add_option(
    "-a, --args",
    args,
    "args for constructing a graph"
  );

  size_t num_threads;
  app.add_option(
    "-t, --num_threads",
    num_threads,
    "number of threads"
  );

  size_t num_streams;
  app.add_option(
    "-s, --num_streams",
    num_streams,
    "number of streams to run. ignore this arg if using TaroCBV2"
  );

  CLI11_PARSE(app, argc, argv);

  Graph* g_ptr;
  if(graph == "SerialGraph") {
    assert(args.size() == 1);
    g_ptr = new SerialGraph(args[0]);
  }
  else if(graph == "ParallelGraph") {
    assert(args.size() == 1);
    g_ptr = new ParallelGraph(args[0]);
  }
  else if(graph == "Tree") {
    assert(args.size() == 2);
    g_ptr = new Tree(args[0], args[1]);
  }
  else if(graph == "RandomDAG") {
    assert(args.size() == 3);
    g_ptr = new RandomDAG(args[0], args[1], args[2]);
  }
  else if(graph == "MapReduce") {
    assert(args.size() == 2);
    g_ptr = new Diamond(args[0], args[1]);
  }
  else if(graph == "Wavefront") {
    assert(args.size() == 1);
    g_ptr = new WavefrontGraph(args[0]);
  }
  else {
    throw std::runtime_error("No such graph\n");
  }

  std::pair<double, double> time_pair;

  GraphExecutor executor(*g_ptr, 0, num_threads, num_streams); 
  time_pair = executor.run(cpu_time, gpu_time);
  
  std::cout << "Construction time: " 
            << time_pair.first
            << " ms\n"
            << "Execution time: "
            << time_pair.second
            << " ms\n";
}

