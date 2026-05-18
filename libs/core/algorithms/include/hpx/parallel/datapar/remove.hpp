//  Copyright (c) 2025 Bhoomish Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR)
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/tag_invoke.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct datapar_remove_if
    {
        template <typename ExPolicy, typename Iter, typename Sent,
            typename Pred, typename Proj>
        static Iter call(
            ExPolicy&&, Iter first, Sent last, Pred&& pred, Proj&& proj)
        {
            using value_type = std::iterator_traits<Iter>::value_type;
            using V = hpx::parallel::traits::vector_pack_type_t<value_type>;
            constexpr std::size_t size =
                hpx::parallel::traits::vector_pack_size_v<V>;

            Iter dest = first;

            while (first != last && !util::detail::is_data_aligned(first))
            {
                if (!HPX_INVOKE(pred, HPX_INVOKE(proj, *first)))
                {
                    if (dest != first)
                    {
                        *dest = std::ranges::iter_move(first);
                    }
                    ++dest;
                }
                ++first;
            }

            // Safety
            while (last - first >= static_cast<std::ptrdiff_t>(size))
            {
                V tmp(hpx::parallel::traits::vector_pack_load<V,
                    value_type>::aligned(first));

                auto msk = HPX_INVOKE(pred, HPX_INVOKE(proj, tmp));

                if (hpx::parallel::traits::none_of(msk))
                {
                    // no elements match
                    if (dest != first)
                    {
                        if (util::detail::is_data_aligned(dest))
                        {
                            hpx::parallel::traits::vector_pack_store<V,
                                value_type>::aligned(tmp, dest);
                        }
                        else
                        {
                            hpx::parallel::traits::vector_pack_store<V,
                                value_type>::unaligned(tmp, dest);
                        }
                    }
                    std::advance(dest, size);
                }
                else if (!hpx::parallel::traits::all_of(msk))
                {
                    // mixed
                    auto const last_set =
                        hpx::parallel::traits::find_last_of(msk);
                    for (int i = hpx::parallel::traits::find_first_of(msk);
                        i != last_set; ++i)
                    {
                        if (!msk[i])
                        {
                            *dest++ = hpx::parallel::traits::get(tmp, i);
                        }
                    }
                }

                // all elements match
                std::advance(first, size);
            }

            while (first != last)
            {
                if (!HPX_INVOKE(pred, HPX_INVOKE(proj, *first)))
                {
                    if (dest != first)
                        *dest = HPX_MOVE(*first);
                    ++dest;
                }
                ++first;
            }

            return dest;
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename Sent, typename Pred, typename Proj>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE Iter tag_invoke(sequential_remove_if_t,
        ExPolicy&& policy, Iter first, Sent last, Pred&& pred, Proj&& proj)
    {
        if constexpr (hpx::parallel::util::detail::
                          iterator_datapar_compatible_v<Iter>)
        {
            return datapar_remove_if::call(HPX_FORWARD(ExPolicy, policy), first,
                last, HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
        }
        else
        {
            return sequential_remove_if(
                hpx::execution::experimental::to_non_simd(policy), first, last,
                HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct datapar_remove
    {
        template <typename ExPolicy, typename Iter, typename Sent, typename T,
            typename Proj>
        static Iter call(ExPolicy&& policy, Iter first, Sent last,
            T const& value, Proj&& proj)
        {
            return datapar_remove_if::call(
                HPX_FORWARD(ExPolicy, policy), first, last,
                [&value](auto const& a) { return a == value; },
                HPX_FORWARD(Proj, proj));
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename Sent, typename T, typename Proj>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    HPX_HOST_DEVICE HPX_FORCEINLINE Iter tag_invoke(sequential_remove_t,
        ExPolicy&& policy, Iter first, Sent last, T const& value, Proj&& proj)
    {
        if constexpr (hpx::parallel::util::detail::iterator_datapar_compatible<
                          Iter>::value)
        {
            return datapar_remove::call(HPX_FORWARD(ExPolicy, policy), first,
                last, value, HPX_FORWARD(Proj, proj));
        }
        else
        {
            return sequential_remove(
                hpx::execution::experimental::to_non_simd(policy), first, last,
                value, HPX_FORWARD(Proj, proj));
        }
    }
}    // namespace hpx::parallel::detail

#endif
