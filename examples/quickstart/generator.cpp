//  Copyright (c) 2017 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This example demonstrates the use of a one_element_channel to construct a
// generic generator interface.

#include <hpx/hpx_main.hpp>
#include <hpx/include/async.hpp>
#include <hpx/include/iostreams.hpp>
#include <hpx/include/lcos.hpp>

#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
// void sync_generator()
// {
//     hpx::lcos::local::one_element_channel<int> gen;
//
//     hpx::future<void> f =
//         hpx::async([=]() mutable -> void
//         {
//             for (int i = 0; i != 10; ++i)
//                 gen.set(i);
//             gen.close();
//         });
//
//     for (int val : gen)
//         hpx::cout << val << '\n';
//
//     f.get();
// }
//
// void async_generator()
// {
//     hpx::lcos::local::one_element_channel<int> gen;
//
//     hpx::future<void> f =
//         hpx::async([=]() mutable -> void
//         {
//             for (int i = 0; i != 10; ++i)
//                 gen.set(i);
//             gen.close();
//         });
//
//     for (hpx::future<int> val : gen.range(hpx::launch::async))
//         hpx::cout << val.get() << '\n';
//
//     f.get();
// }

namespace hpx
{
//     template <typename T, typename F>
//     hpx::generator<T> make_generator(F && f)
//     {
//         hpx::generator<T> gen;
//         hpx::apply(
//             [&, gen]() mutable -> void
//             {
//                 f(gen);
//                 gen.close();
//             });
//         return gen;
//     }

    template <typename T, typename F>
    hpx::generator<T> make_generator(F && f)
    {
        hpx::apply(
            [&]() mutable -> void
            {
                f();
            });
        return gen;
    }
}

// ///////////////////////////////////////////////////////////////////////////////
// void sync_wrapped_generator()
// {
//     // create and launch generator
//     hpx::generator<int> gen =
//         hpx::make_generator<int>(
//             [](hpx::generator<int> g) -> void
//             {
//                 for (int i = 0; i != 10; ++i)
//                     g.set(i);
//             });
//
//     // retrieve values from generator
//     for (int val : gen)
//         hpx::cout << val << '\n';
// }
//
// void async_wrapped_generator()
// {
//     // create and launch generator
//     hpx::generator<int> gen =
//         hpx::make_generator<int>(
//             [](hpx::generator<int> g) -> void
//             {
//                 for (int i = 0; i != 10; ++i)
//                     g.set(i);
//             });
//
//     // asynchronously retrieve values from generator
//     for (hpx::future<int> val : gen.range(hpx::launch::async))
//         hpx::cout << val.get() << '\n';
// }

///////////////////////////////////////////////////////////////////////////////
hpx::generator<int> await_wrapped_generator()
{
    for (int i = 0; i != 10; ++i)
    {
        hpx::cout << "1: " << i << ", " << hpx::threads::get_self_id() << '\n';
        co_yield(i);
    }
}

void await_wrapped_consumer()
{
    for (int val : await_wrapped_generator())
    {
        hpx::cout << "2: " << val << ", " << hpx::threads::get_self_id() << '\n';
        hpx::cout << val << '\n';
    }
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
//     sync_generator();
//     async_generator();
//     sync_wrapped_generator();
//     async_wrapped_generator();

    await_wrapped_consumer();


    return 0;
}
