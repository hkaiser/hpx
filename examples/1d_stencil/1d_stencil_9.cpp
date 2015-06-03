//  Copyright (c) 2014-2015 Hartmut Kaiser
//  Copyright (c) 2014 Patricia Grubel
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This is the ninth in a series of examples demonstrating the development of
// a fully distributed solver for a simple 1D heat distribution problem.
//
// This example builds on example four. While example four is designed for SMP
// systems, this example utilizes hpx::vector, a distributed data structure to
// achieve a similarly concise code but working for a distributed environment.

#include <hpx/hpx_init.hpp>
#include <hpx/hpx.hpp>

#include <hpx/include/vector.hpp>
#include <hpx/include/parallel_algorithm.hpp>

#include <boost/range/irange.hpp>

#include "print_time_results.hpp"

///////////////////////////////////////////////////////////////////////////////
// Define the vector types to be used.
HPX_REGISTER_VECTOR(double);

///////////////////////////////////////////////////////////////////////////////
// Command-line variables
bool header = true; // print csv heading
double k = 0.5;     // heat transfer coefficient
double dt = 1.;     // time step
double dx = 1.;     // grid spacing

// inline std::size_t idx(std::size_t i, int dir, std::size_t size)
// {
//     if(i == 0 && dir == -1)
//         return size-1;
//     if(i == size-1 && dir == +1)
//         return 0;
//
//     HPX_ASSERT((i + dir) < size);
//
//     return i + dir;
// }

///////////////////////////////////////////////////////////////////////////////
// Our partition data type
// struct partition_data
// {
// public:
//     partition_data(std::size_t size)
//       : data_(new double[size]), size_(size)
//     {}
//
//     partition_data(std::size_t size, double initial_value)
//       : data_(new double[size]),
//         size_(size)
//     {
//         double base_value = double(initial_value * size);
//         for (std::size_t i = 0; i != size; ++i)
//             data_[i] = base_value + double(i);
//     }
//
//     partition_data(partition_data && other)
//       : data_(std::move(other.data_))
//       , size_(other.size_)
//     {}
//
//     double& operator[](std::size_t idx) { return data_[idx]; }
//     double operator[](std::size_t idx) const { return data_[idx]; }
//
//     std::size_t size() const { return size_; }
//
// private:
//     std::unique_ptr<double[]> data_;
//     std::size_t size_;
//
//     HPX_MOVABLE_BUT_NOT_COPYABLE(partition_data);
// };
//
// std::ostream& operator<<(std::ostream& os, partition_data const& c)
// {
//     os << "{";
//     for (std::size_t i = 0; i != c.size(); ++i)
//     {
//         if (i != 0)
//             os << ", ";
//         os << c[i];
//     }
//     os << "}";
//     return os;
// }

///////////////////////////////////////////////////////////////////////////////
struct stepper
{
    // Our data for one time step
    typedef hpx::vector<double> space;

    // Our operator
//     static double heat(double left, double middle, double right)
//     {
//         return middle + (k*dt/dx*dx) * (left - 2*middle + right);
//     }

    // The partitioned operator, it invokes the heat operator above on all
    // elements of a partition.
//     static partition_data heat_part(partition_data const& left,
//         partition_data const& middle, partition_data const& right)
//     {
//         std::size_t size = middle.size();
//         partition_data next(size);
//
//         next[0] = heat(left[size-1], middle[0], middle[1]);
//
//         for(std::size_t i = 1; i != size-1; ++i)
//         {
//             next[i] = heat(middle[i-1], middle[i], middle[i+1]);
//         }
//
//         next[size-1] = heat(middle[size-2], middle[size-1], right[0]);
//
//         return next;
//     }

    // do all the work on 'np' partitions, 'nx' data points each, for 'nt'
    // time steps
    space do_work(std::size_t np, std::size_t nx, std::size_t nt)
    {
        using hpx::lcos::local::dataflow;
        using hpx::util::unwrapped;
        using hpx::util::zip_iterator;
        using hpx::util::make_zip_iterator;

        // U[t][i] is the state of position i at time t.
        std::vector<space> U(2);

        // connect to the created vectors of data points
        U[0].connect_to("U1");
        U[1].connect_to("U2");

        // Initial conditions: f(0, i) = i
        typedef boost::range_const_iterator<
                boost::integer_range<std::size_t>
            >::type range_iterator_type;
        typedef zip_iterator<
                range_iterator_type, hpx::vector<double>::local_iterator
            > zip_iterator_type;
        typedef zip_iterator_type::reference reference;

        std::size_t b = 0;
        auto range = boost::irange(b, np*nx);
        hpx::parallel::for_each(
            hpx::parallel::par,
            make_zip_iterator(boost::begin(range), boost::begin(U[0])),
            make_zip_iterator(boost::end(range), boost::end(U[0])),
            [](reference t)
            {
                using hpx::util::get;
                get<1>(t) = double(get<0>(t));
            }
        );

//         auto Op = unwrapped(&stepper::heat_part);
//
//         // Actual time step loop
//         for (std::size_t t = 0; t != nt; ++t)
//         {
//             space const& current = U[t % 2];
//             space& next = U[(t + 1) % 2];
//
//             for (std::size_t i = 0; i != np; ++i)
//             {
//                 next[i] = dataflow(
//                         hpx::launch::async, Op,
//                         current[idx(i, -1, np)], current[i], current[idx(i, +1, np)]
//                     );
//             }
//         }
//
//         // Return the solution at time-step 'nt'.
//         return hpx::when_all(U[nt % 2]);
        return U[0];
    }
};

///////////////////////////////////////////////////////////////////////////////
int hpx_main(boost::program_options::variables_map& vm)
{
    boost::uint64_t np = vm["np"].as<boost::uint64_t>();   // Number of partitions.
    boost::uint64_t nx = vm["nx"].as<boost::uint64_t>();   // Number of grid points.
    boost::uint64_t nt = vm["nt"].as<boost::uint64_t>();   // Number of steps.

    if (vm.count("no-header"))
        header = false;

    // Create the vectors on locality 0 only, all others will connect to it in
    // the stepper
    std::vector<hpx::id_type> localities = hpx::find_all_localities();

    hpx::vector<double> U1(nx*np, hpx::container_layout(np, localities));
    U1.register_as("U1");
    hpx::vector<double> U2(nx*np, hpx::container_layout(np, localities));
    U2.register_as("U2");

    // Create the stepper object
    stepper step;

//     // Measure execution time.
//     boost::uint64_t t = hpx::util::high_resolution_clock::now();
//
//     // Execute nt time steps on nx grid points and print the final solution.
//     hpx::future<stepper::space> result = step.do_work(np, nx, nt);
//
//     stepper::space solution = result.get();
//     hpx::wait_all(solution);
//
//     boost::uint64_t elapsed = hpx::util::high_resolution_clock::now() - t;
//
//     // Print the final solution
//     if (vm.count("results"))
//     {
//         for (std::size_t i = 0; i != np; ++i)
//             std::cout << "U[" << i << "] = " << solution[i].get() << std::endl;
//     }
//
//     boost::uint64_t const os_thread_count = hpx::get_os_thread_count();
//     print_time_results(os_thread_count, elapsed, nx, np, nt, header);

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    using namespace boost::program_options;

    // Configure application-specific options.
    options_description desc_commandline;

    desc_commandline.add_options()
        ("results", "print generated results (default: false)")
        ("nx", value<boost::uint64_t>()->default_value(10),
         "Local x dimension (of each partition)")
        ("nt", value<boost::uint64_t>()->default_value(45),
         "Number of time steps")
        ("np", value<boost::uint64_t>()->default_value(10),
         "Number of partitions")
        ("k", value<double>(&k)->default_value(0.5),
         "Heat transfer coefficient (default: 0.5)")
        ("dt", value<double>(&dt)->default_value(1.0),
         "Timestep unit (default: 1.0[s])")
        ("dx", value<double>(&dx)->default_value(1.0),
         "Local x dimension")
        ( "no-header", "do not print out the csv header row")
    ;

    // Initialize and run HPX, this example requires to run hpx_main on all
    // localities
    std::vector<std::string> cfg;
    cfg.push_back("hpx.run_hpx_main!=1");

    // Initialize and run HPX
    return hpx::init(desc_commandline, argc, argv, cfg);
}
