//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2023 The STE||AR Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>
#include <vector>

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#if !defined(HPX_MODULE_STATIC_LINKING)
#if defined(HPX_CORE_EXPORTS)
#define HPX_CONFIG_REGISTRY_EXPORT __declspec(dllexport)
#else
#define HPX_CONFIG_REGISTRY_EXPORT __declspec(dllimport)
#endif
#endif
#elif defined(__NVCC__) || defined(__CUDACC__)
#define HPX_CONFIG_REGISTRY_EXPORT /* empty */
#elif defined(HPX_CORE_EXPORTS)
#define HPX_CONFIG_REGISTRY_EXPORT __attribute__((visibility("default")))
#endif

// make sure we have reasonable defaults
#if !defined(HPX_CONFIG_REGISTRY_EXPORT)
#define HPX_CONFIG_REGISTRY_EXPORT /* empty */
#endif

#if !defined(HPX_CPP_EXPORT)
#define HPX_CPP_EXPORT
#endif
#if !defined(HPX_CPP_EXPORT_EXTERN)
#define HPX_CPP_EXPORT_EXTERN extern "C++"
#endif

namespace hpx::config_registry {

    HPX_CPP_EXPORT struct module_config
    {
        std::string module_name;
        std::vector<std::string> config_entries;
    };

    HPX_CPP_EXPORT_EXTERN [[nodiscard]] HPX_CONFIG_REGISTRY_EXPORT
        std::vector<module_config> const&
        get_module_configs();
    HPX_CPP_EXPORT_EXTERN HPX_CONFIG_REGISTRY_EXPORT void add_module_config(
        module_config const& config);

    HPX_CPP_EXPORT_EXTERN struct HPX_CONFIG_REGISTRY_EXPORT add_module_config_helper
    {
        explicit add_module_config_helper(module_config const& config);
    };
}    // namespace hpx::config_registry

namespace hpx::config_registry_cfg {

    HPX_CPP_EXPORT_EXTERN HPX_CONFIG_REGISTRY_EXPORT
        config_registry::add_module_config_helper add_config;
}    // namespace hpx::config_registry_cfg
