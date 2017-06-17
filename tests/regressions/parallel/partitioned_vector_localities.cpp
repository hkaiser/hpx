//  Copyright (c) 2017 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/iostreams.hpp>

#include <hpx/include/parallel_for_each.hpp>
#include <hpx/include/partitioned_vector.hpp>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

///////////////////////////////////////////////////////////////////////////////
// Define the vector types to be used.
HPX_REGISTER_PARTITIONED_VECTOR(int);

///////////////////////////////////////////////////////////////////////////////
struct print_here
{
    void operator()(int val) const
    {
        hpx::cout << hpx::find_here() << hpx::endl;
    }
};

///////////////////////////////////////////////////////////////////////////////
int hpx_main(int argc, char* argv[])
{
    hpx::partitioned_vector<int> values(100,
        hpx::container_layout(hpx::find_all_localities()));

    // expected to see the locality id of each element in values get printed
    hpx::future<void> fwait =
        hpx::parallel::for_each(
            hpx::parallel::execution::par(hpx::parallel::execution::task),
            boost::begin(values), boost::end(values),
            print_here());
    fwait.wait();

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    return hpx::init(argc, argv);
}
