#include <hpx/hpx_main.hpp>
#include <hpx/iostream.hpp>
#include <iostream>
#include <hpx/algorithm.hpp>

#include <thrust/device_vector.h>

#include <hpx/components/containers/partitioned_vector_thrust/partitioned_vector_thrust.hpp>
#include <hpx/include/partitioned_vector.hpp>
#include <hpx/modules/thrust.hpp>

    HPX_REGISTER_PARTITIONED_VECTOR(int, thrust::device_vector<int>, int_device_vector);

int main(int argc, char** argv)
{
    hpx::partitioned_vector<int, thrust::device_vector<int>> v(100);

    // test 1 (compiles but won't run)
    v.set_value(0,42);
    int answer = v.get_value(0).get();
    hpx::cout << "Answer is: " << answer << std::endl;

    // test 2 (will not compile)
    //hpx::fill(hpx::thrust::thrust_device_policy{}, v.begin(), v.end(), 42);

    // test 3 (compiles but won't run)
    hpx::thrust::segmented_fill(hpx::thrust::thrust_device_policy{}, v.begin(), v.end(), 42);

    hpx::cout << "Answer is: " << v.get_value(0).get() << std::endl;


    return 0;
}
