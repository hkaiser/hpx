//  Copyright (c) 2021 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/executors/thread_pool_executor.hpp>
#include <hpx/local/algorithm.hpp>
#include <hpx/local/chrono.hpp>
#include <hpx/local/execution.hpp>
#include <hpx/local/init.hpp>
#include <hpx/local/runtime.hpp>

#include "worker_timed.hpp"

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
template <typename Executor>
void measure_executor_once(
    Executor&& exec, int num_timesteps, int num_iterations, int delay)
{
    hpx::execution::static_chunk_size cs(1);
    for (int i = 0; i != num_timesteps; ++i)
    {
        hpx::for_loop(hpx::execution::par.on(exec).with(cs), 0,
            std::size_t(num_iterations),
            [&](std::size_t i) { worker_timed(delay); });
    }
}

template <typename Executor>
std::uint64_t measure_executor(Executor&& exec, int test_count,
    int num_timesteps, int num_iterations, int delay)
{
    std::uint64_t start = hpx::chrono::high_resolution_clock::now();
    for (int i = 0; i != test_count; ++i)
    {
        measure_executor_once(exec, num_timesteps, num_iterations, delay);
    }
    return (hpx::chrono::high_resolution_clock::now() - start) / test_count;
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    int test_count = vm["test_count"].as<int>();
    int num_timesteps = vm["num_timesteps"].as<int>();
    int num_iterations = static_cast<int>(hpx::get_num_worker_threads());
    int work_delay = vm["work_delay"].as<int>();
    bool csvoutput = vm.count("csv_output") != 0;

    std::uint64_t time_default = 0;
    std::uint64_t time_pool = 0;

    {
        hpx::execution::parallel_executor exec;
        time_default = measure_executor(
            exec, test_count, num_timesteps, num_iterations, work_delay);
    }

    {
        hpx::execution::thread_pool_executor exec(num_iterations);
        time_pool = measure_executor(
            exec, test_count, num_timesteps, num_iterations, work_delay);
    }

    if (csvoutput)
    {
        std::cout << "test_count,num_timesteps,num_iterations,work_delay,"
                     "time_default,time_pool\n";
        std::cout << test_count << "," << num_timesteps << "," << num_iterations
                  << "," << work_delay << "," << time_default / 1e9 << ","
                  << time_pool / 1e9 << "\n";
    }
    else
    {
        std::cout << std::left
                  << "----------------Parameters---------------------\n"
                  << std::left
                  << "Number of threads                 : " << std::right
                  << std::setw(8) << num_iterations << "\n"
                  << std::left
                  << "Number of tests                   : " << std::right
                  << std::setw(8) << test_count << "\n"
                  << std::left
                  << "Number of timesteps               : " << std::right
                  << std::setw(8) << num_timesteps << "\n"
                  << std::left
                  << "Delay per iteration (nanoseconds) : " << std::right
                  << std::setw(8) << work_delay << "\n";

        std::cout << "-----------------------------------------------\n"
                  << std::left
                  << "Average execution time (par)      : " << std::right
                  << std::setw(8) << time_default / 1e9 << "\n";

        std::cout << std::left
                  << "Average execution time (pool)     : " << std::right
                  << std::setw(8) << time_pool / 1e9 << "\n";
    }

    return hpx::local::finalize();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    //initialize program
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    using namespace hpx::program_options;

    options_description cmdline("usage: " HPX_APPLICATION_STRING " [options]");

    // clang-format off
    cmdline.add_options()
        ("num_timesteps", value<int>()->default_value(1000),
            "number of timesteps")
        ("work_delay", value<int>()->default_value(0),
            "loop delay per element in nanoseconds")
        ("test_count", value<int>()->default_value(100),
            "number of tests to be averaged")
        ("csv_output", "print results in csv format")
        ;
    // clang-format on

    hpx::local::init_params init_args;
    init_args.desc_cmdline = cmdline;
    init_args.cfg = cfg;

    return hpx::local::init(hpx_main, argc, argv, init_args);
}

#endif
