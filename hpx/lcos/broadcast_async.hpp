//  Copyright (c) 2013-2017 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file broadcast_async.hpp

#ifndef HPX_LCOS_BROADCAST_ASYNC_HPP
#define HPX_LCOS_BROADCAST_ASYNC_HPP

#include <hpx/config.hpp>
#include <hpx/async.hpp>
#include <hpx/lcos/dataflow.hpp>
#include <hpx/lcos/future.hpp>
#include <hpx/lcos/promise.hpp>
#include <hpx/lcos/when_all.hpp>
#include <hpx/runtime/actions/plain_action.hpp>
#include <hpx/runtime/agas/symbol_namespace.hpp>
#include <hpx/runtime/basename_registration.hpp>
#include <hpx/runtime/naming/id_type.hpp>
#include <hpx/runtime/trigger_lco.hpp>
#include <hpx/traits/acquire_shared_state.hpp>
#include <hpx/traits/future_access.hpp>
#include <hpx/util/bind.hpp>
#include <hpx/util/deferred_call.hpp>
#include <hpx/util/calculate_fanout.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/format.hpp>
#include <boost/preprocessor/cat.hpp>

#if !defined(HPX_BROADCAST_FANOUT)
#define HPX_BROADCAST_FANOUT 16
#endif

namespace hpx { namespace lcos
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    hpx::future<T> broadcast_recv(char const* basename,
        std::size_t this_site = std::size_t(-1),
        std::size_t generation = std::size_t(-1))
    {
        if (this_site == std::size_t(-1))
            this_site = static_cast<std::size_t>(hpx::get_locality_id());

        std::string name(basename);
        if (generation != std::size_t(-1))
            name += std::to_string(generation) + "/";

        typedef typename std::decay<T>::type result_type;

        // this is the receiving endpoint for this site
        hpx::lcos::promise<result_type> p;
        hpx::future<result_type> f = p.get_future();

        // register promise using symbolic name
        hpx::future<bool> was_registered =
            hpx::register_with_basename(name, p.get_id(), this_site);

        return hpx::dataflow(hpx::launch::sync,
            [](hpx::future<result_type> f, hpx::future<bool> was_registered,
                std::string && name, std::size_t this_site) -> result_type
            {
                // rethrow exceptions
                was_registered.get();

                // make sure promise gets unregistered after use
                hpx::unregister_with_basename(name, this_site).get();

                // propagate result
                return f.get();
            },
            std::move(f), std::move(was_registered), std::move(name), this_site);
    }

    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        ///////////////////////////////////////////////////////////////////////
        std::map<std::uint32_t, std::vector<std::size_t> >
        generate_locality_indices(std::string const& name, std::size_t num_sites)
        {
            std::map<std::uint32_t, std::vector<std::size_t> > indices;
            for (std::size_t i = 0; i != num_sites; ++i)
            {
                std::uint32_t locality_id =
                    agas::symbol_namespace::service_locality_id(
                        hpx::detail::name_from_basename(name, i));
                indices[locality_id].push_back(i);
            }
            return indices;
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename T>
        hpx::future<void> broadcast_send_invoke(std::string const& name,
            std::size_t site, T && t)
        {
            // this should be always executed on the locality responsible for
            // resolving the given name
            HPX_ASSERT(
                agas::symbol_namespace::service_locality_id(
                    hpx::detail::name_from_basename(name, site)) ==
                hpx::get_locality_id()
            );

            // find_from_basename is always a local operation (see assert above)
            typedef typename std::decay<T>::type result_type;
            return hpx::dataflow(hpx::launch::sync,
                [](hpx::future<hpx::id_type> f, result_type && t) -> void
                {
                    set_lco_value(f.get(), std::move(t));
                },
                hpx::find_from_basename(name, site), std::forward<T>(t));
        }

        template <typename T>
        struct broadcast_send
        {
            static hpx::future<void> call(std::string const& name,
                std::vector<std::size_t> const& sites, T && t)
            {
                if (sites.empty())
                    return hpx::make_ready_future();

                // apply actual broadcast operation to set of sites managed
                // on this locality
                if (sites.size() == 1)
                {
                    return detail::broadcast_send_invoke(
                        name, sites[0], std::move(t));
                }

                std::vector<hpx::future<void> > futures;
                futures.reserve(sites.size());
                for(std::size_t i : sites)
                {
                    futures.push_back(detail::broadcast_send_invoke(name, i, t));
                }
                return hpx::when_all(futures);
            }
        };

        template <typename T>
        struct broadcast_send_invoke_action
        {
            typedef typename HPX_MAKE_ACTION(broadcast_send<T>::call)::type type;
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename T>
        hpx::future<void> broadcast_send(std::string const& name, T && t,
            std::map<std::uint32_t, std::vector<std::size_t> > const& sites,
            std::size_t global_idx);

        template <typename T>
        struct broadcast_tree_send
        {
            static hpx::future<void> call(std::string const& name, T && t,
                std::map<std::uint32_t, std::vector<std::size_t> > const& sites,
                std::size_t global_idx)
            {
                return broadcast_send(name, std::forward<T>(t), sites, global_idx);
            }
        };

        template <typename T>
        struct broadcast_tree_send_invoke_action
        {
            typedef typename HPX_MAKE_ACTION(broadcast_tree_send<T>::call)::type type;
        };

        ///////////////////////////////////////////////////////////////////////
        std::map<std::uint32_t, std::vector<std::size_t> >
        get_next_locality_slice(
            std::map<std::uint32_t, std::vector<std::size_t> >::iterator it,
            std::size_t slice)
        {
            typedef std::map<std::uint32_t, std::vector<std::size_t> > indices_type;

            indices_type next_localities;
            while (slice-- != 0)
            {
                next_localities.insert(std::move(*it));
                ++it;
            }
            return next_localities;
        }

        template <typename T>
        hpx::future<void> broadcast_send(std::string const& name, T && t,
            std::map<std::uint32_t, std::vector<std::size_t> > const& sites,
            std::size_t global_idx)
        {
            typedef typename lcos::detail::broadcast_send_invoke_action<
                    typename std::decay<T>::type
                >::type broadcast_send_action;

            std::size size = sites.size();
            if (size == 1)
            {
                return hpx::async(broadcast_send_action(),
                    hpx::naming::get_id_from_locality_id(
                        sites.begin()->first),
                    std::move(name),
                    std::move(sites.begin()->second),
                    std::forward<T>(t));
            }

            std::size_t const local_fanout = HPX_BROADCAST_FANOUT;
            std::size_t local_size = (std::min)(size, local_fanout);
            std::size_t fanout = util::calculate_fanout(size, local_fanout);

            std::vector<hpx::future<void> > futures;
            futures.reserve(local_size + (size / fanout) + 1);

            // the first HPX_BROADCAST_FANOUT targets are handled directly
            for(std::size_t i = 0; i != local_size; ++i)
            {
                futures.push_back(hpx::async(broadcast_send_action(),
                    hpx::naming::get_id_from_locality_id(sites[i].first),
                    name, std:move(p.second), t));
            }

            // the remaining targets are triggered using a tree-style broadcast
            typedef typename lcos::detail::broadcast_tree_invoke_action<
                    typename std::decay<T>::type
                >::type broadcast_tree_action;

            if (local_size > local_fanout)
            {
                std::size_t applied = local_fanout;
                typename indices_type::const_iterator it = sites.begin();
                std::advance(it, local_fanout);

                while(it != sites.end())
                {
                    HPX_ASSERT(size >= applied);

                    std::size_t next_fan = (std::min)(fanout, size - applied);
                    indices_type next_sites =
                        detail::get_next_locality_slice(it, next_fan);

                    hpx::id_type id();

                    futures.push_back(hpx::async(broadcast_tree_action(),
                        hpx::naming::get_id_from_locality_id(next_sites.begin()->first),
                        std::move(next_sites), global_idx + applied, t)
                    );

                    applied += next_fan;
                    it += next_fan;
                }

            }

            return hpx::when_all(futures);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    hpx::future<void> broadcast_send(char const* basename, T && t,
        std::size_t num_sites = std::size_t(-1),
        std::size_t generation = std::size_t(-1))
    {
        if (num_sites == std::size_t(-1))
            num_sites = hpx::get_num_localities(hpx::launch::sync);

        std::string name(basename);
        if (generation != std::size_t(-1))
            name += std::to_string(generation) + "/";

        // generate mapping of which sites are managed by what symbol namespace
        // instances
        typedef std::map<std::uint32_t, std::vector<std::size_t> > indices_type;
        indices_type locality_indices =
            detail::generate_locality_indices(name, num_sites);

        return detail::broadcast_send(std::move(locality_indices),
            std::move(name));
    }
}}

#define HPX_BROADCAST_ASYNC_DECLARATION(Type)                                 \
    HPX_REGISTER_ACTION_DECLARATION(                                          \
        hpx::lcos::detail::broadcast_send_invoke_action< Type>::type,         \
        BOOST_PP_CAT(broadcast_async_, Type))                                 \
/**/
#define HPX_BROADCAST_ASYNC(Type)                                             \
    HPX_REGISTER_ACTION(                                                      \
        hpx::lcos::detail::broadcast_send_invoke_action< Type>::type,         \
        BOOST_PP_CAT(broadcast_async_, Type))                                 \
/**/

#endif
