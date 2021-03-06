//  Copyright (c)      2018 Mikael Simberg
//  Copyright (c) 2007-2016 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx_start.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/functional/function.hpp>
#include <hpx/hpx_finalize.hpp>
#include <hpx/hpx_init_params.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/runtime_configuration/runtime_mode.hpp>
#include <hpx/runtime_local/shutdown_function.hpp>
#include <hpx/runtime_local/startup_function.hpp>

#include <cstddef>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
/// \namespace hpx
namespace hpx {
    /// \brief Main non-blocking entry point for launching the HPX runtime system.
    ///
    /// This is the main, non-blocking entry point for any HPX application.
    /// This function (or one of its overloads below) should be called from
    /// the users `main()` function. It will set up the HPX runtime environment
    /// and schedule the function given by \p f as a HPX thread. It will return
    /// immediately after that. Use `hpx::wait` and `hpx::stop` to synchronize
    /// with the runtime system's execution. This overload will not call
    /// `hpx_main`.
    ///
    /// \param f            [in] The function to be scheduled as an HPX
    ///                     thread. Usually this function represents the main
    ///                     entry point of any HPX application. If \p f is
    ///                     `nullptr` the HPX runtime environment will be started
    ///                     without invoking \p f.
    /// \param argc         [in] The number of command line arguments passed
    ///                     in \p argv. This is usually the unchanged value as
    ///                     passed by the operating system (to `main()`).
    /// \param argv         [in] The command line arguments for this
    ///                     application, usually that is the value as passed
    ///                     by the operating system (to `main()`).
    /// \param params       [in] The parameters to the \a hpx::start function
    ///                     (See documentation of \a hpx::init_params)
    ///
    /// \returns            The function returns \a true if command line processing
    ///                     succeeded and the runtime system was started successfully.
    ///                     It will return \a false otherwise.
    ///
    /// \note               If the parameter \p mode is not given (defaulted),
    ///                     the created runtime system instance will be
    ///                     executed in console or worker mode depending on the
    ///                     command line arguments passed in `argc`/`argv`.
    ///                     Otherwise it will be executed as specified by the
    ///                     parameter\p mode.
    inline bool start(
        util::function_nonser<int(hpx::program_options::variables_map&)> const&
            f,
        int argc, char** argv, init_params const& params = init_params());

    /// \brief Main non-blocking entry point for launching the HPX runtime system.
    ///
    /// This is the main, non-blocking entry point for any HPX application.
    /// This function (or one of its overloads below) should be called from
    /// the users `main()` function. It will set up the HPX runtime environment
    /// and schedule the function given by \p f as a HPX thread. It will return
    /// immediately after that. Use `hpx::wait` and `hpx::stop` to synchronize
    /// with the runtime system's execution. This overload will not call
    /// `hpx_main`.
    ///
    /// \param f            [in] The function to be scheduled as an HPX
    ///                     thread. Usually this function represents the main
    ///                     entry point of any HPX application. If \p f is
    ///                     `nullptr` the HPX runtime environment will be started
    ///                     without invoking \p f.
    /// \param argc         [in] The number of command line arguments passed
    ///                     in \p argv. This is usually the unchanged value as
    ///                     passed by the operating system (to `main()`).
    /// \param argv         [in] The command line arguments for this
    ///                     application, usually that is the value as passed
    ///                     by the operating system (to `main()`).
    /// \param params       [in] The parameters to the \a hpx::start function
    ///                     (See documentation of \a hpx::init_params)
    ///
    /// \returns            The function returns \a true if command line processing
    ///                     succeeded and the runtime system was started successfully.
    ///                     It will return \a false otherwise.
    ///
    /// \note               If the parameter \p mode is not given (defaulted),
    ///                     the created runtime system instance will be
    ///                     executed in console or worker mode depending on the
    ///                     command line arguments passed in `argc`/`argv`.
    ///                     Otherwise it will be executed as specified by the
    ///                     parameter\p mode.
    inline bool start(util::function_nonser<int(int, char**)> const& f,
        int argc, char** argv, init_params const& params = init_params());

    /// \brief Main non-blocking entry point for launching the HPX runtime system.
    ///
    /// This is the main, non-blocking entry point for any HPX application.
    /// This function (or one of its overloads below) should be called from
    /// the users `main()` function. It will set up the HPX runtime environment
    /// and schedule the function given by \p f as a HPX thread. It will return
    /// immediately after that. Use `hpx::wait` and `hpx::stop` to synchronize
    /// with the runtime system's execution. This overload will not call
    /// `hpx_main`.
    ///
    /// \param argc         [in] The number of command line arguments passed
    ///                     in \p argv. This is usually the unchanged value as
    ///                     passed by the operating system (to `main()`).
    /// \param argv         [in] The command line arguments for this
    ///                     application, usually that is the value as passed
    ///                     by the operating system (to `main()`).
    /// \param params       [in] The parameters to the \a hpx::start function
    ///                     (See documentation of \a hpx::init_params)
    ///
    /// \returns            The function returns \a true if command line processing
    ///                     succeeded and the runtime system was started successfully.
    ///                     It will return \a false otherwise.
    ///
    /// \note               If the parameter \p mode is not given (defaulted),
    ///                     the created runtime system instance will be
    ///                     executed in console or worker mode depending on the
    ///                     command line arguments passed in `argc`/`argv`.
    ///                     Otherwise it will be executed as specified by the
    ///                     parameter\p mode.
    inline bool start(
        int argc, char** argv, init_params const& params = init_params());

    /// \brief Main non-blocking entry point for launching the HPX runtime system.
    ///
    /// This is the main, non-blocking entry point for any HPX application.
    /// This function (or one of its overloads below) should be called from
    /// the users `main()` function. It will set up the HPX runtime environment
    /// and schedule the function given by \p f as a HPX thread. It will return
    /// immediately after that. Use `hpx::wait` and `hpx::stop` to synchronize
    /// with the runtime system's execution. This overload will not call
    /// `hpx_main`.
    ///
    /// \param f            [in] The function to be scheduled as an HPX
    ///                     thread. Usually this function represents the main
    ///                     entry point of any HPX application. If \p f is
    ///                     `nullptr` the HPX runtime environment will be started
    ///                     without invoking \p f.
    /// \param argc         [in] The number of command line arguments passed
    ///                     in \p argv. This is usually the unchanged value as
    ///                     passed by the operating system (to `main()`).
    /// \param argv         [in] The command line arguments for this
    ///                     application, usually that is the value as passed
    ///                     by the operating system (to `main()`).
    /// \param params       [in] The parameters to the \a hpx::start function
    ///                     (See documentation of \a hpx::init_params)
    ///
    /// \returns            The function returns \a true if command line processing
    ///                     succeeded and the runtime system was started successfully.
    ///                     It will return \a false otherwise.
    ///
    /// \note               If the parameter \p mode is not given (defaulted),
    ///                     the created runtime system instance will be
    ///                     executed in console or worker mode depending on the
    ///                     command line arguments passed in `argc`/`argv`.
    ///                     Otherwise it will be executed as specified by the
    ///                     parameter\p mode.
    inline bool start(std::nullptr_t f, int argc, char** argv,
        init_params const& params = init_params());

    /// \brief Main non-blocking entry point for launching the HPX runtime system.
    ///
    /// This is a simplified main, non-blocking entry point, which can be used
    /// to set up the runtime for an HPX application (the runtime system will
    /// be set up in console mode or worker mode depending on the command line
    /// settings). It will return immediately after that. Use `hpx::wait` and
    /// `hpx::stop` to synchronize with the runtime system's execution.
    ///
    /// \param params       [in] The parameters to the \a hpx::start function
    ///                     (See documentation of \a hpx::init_params)
    ///
    /// \returns            The function returns \a true if command line processing
    ///                     succeeded and the runtime system was started successfully.
    ///                     It will return \a false otherwise.
    ///
    /// \note               The created runtime system instance will be
    ///                     executed in console or worker mode depending on the
    ///                     command line arguments passed in `argc`/`argv`. If not
    ///                     command line arguments are passed, console mode is
    ///                     assumed.
    ///
    /// \note               If no command line arguments are passed the HPX
    ///                     runtime system will not support any of the default
    ///                     command line options as described in the section
    ///                     'HPX Command Line Options'.
    inline bool start(init_params const& params = init_params());

#if defined(HPX_HAVE_INIT_START_OVERLOADS_COMPATIBILITY)
    /// \brief Main non-blocking entry point for launching the HPX runtime system.
    ///
    /// This is the main, non-blocking entry point for any HPX application.
    /// This function (or one of its overloads below) should be called from
    /// the users `main()` function. It will set up the HPX runtime environment
    /// and schedule the function given by \p f as a HPX thread. It will return
    /// immediately after that. Use `hpx::wait` and `hpx::stop` to synchronize
    /// with the runtime system's execution. This overload will not call
    /// `hpx_main`.
    ///
    /// \param f            [in] The function to be scheduled as an HPX
    ///                     thread. Usually this function represents the main
    ///                     entry point of any HPX application. If \p f is
    ///                     `nullptr` the HPX runtime environment will be started
    ///                     without invoking \p f.
    /// \param desc_cmdline [in] This parameter may hold the description of
    ///                     additional command line arguments understood by the
    ///                     application. These options will be prepended to
    ///                     the default command line options understood by
    ///                     \a hpx::init (see description below).
    /// \param argc         [in] The number of command line arguments passed
    ///                     in \p argv. This is usually the unchanged value as
    ///                     passed by the operating system (to `main()`).
    /// \param argv         [in] The command line arguments for this
    ///                     application, usually that is the value as passed
    ///                     by the operating system (to `main()`).
    /// \param cfg          A list of configuration settings which will be added
    ///                     to the system configuration before the runtime
    ///                     instance is run. Each of the entries in this list
    ///                     must have the format of a fully defined key/value
    ///                     pair from an ini-file (for instance
    ///                     'hpx.component.enabled=1')
    /// \param startup      [in] A function to be executed inside a HPX
    ///                     thread before \p f is called. If this parameter
    ///                     is not given no function will be executed.
    /// \param shutdown     [in] A function to be executed inside an HPX
    ///                     thread while hpx::finalize is executed. If this
    ///                     parameter is not given no function will be
    ///                     executed.
    /// \param mode         [in] The mode the created runtime environment
    ///                     should be initialized in. There has to be exactly
    ///                     one locality in each HPX application which is
    ///                     executed in console mode (\a hpx::runtime_mode::console),
    ///                     all other localities have to be run in worker mode
    ///                     (\a hpx::runtime_mode::worker). Normally this is
    ///                     set up automatically, but sometimes it is necessary
    ///                     to explicitly specify the mode.
    ///
    /// \returns            The function returns \a true if command line processing
    ///                     succeeded and the runtime system was started successfully.
    ///                     It will return \a false otherwise.
    ///
    /// \note               If the parameter \p mode is not given (defaulted),
    ///                     the created runtime system instance will be
    ///                     executed in console or worker mode depending on the
    ///                     command line arguments passed in `argc`/`argv`.
    ///                     Otherwise it will be executed as specified by the
    ///                     parameter\p mode.
    inline bool start(util::function_nonser<int(
                          hpx::program_options::variables_map& vm)> const& f,
        hpx::program_options::options_description const& desc_cmdline, int argc,
        char** argv, std::vector<std::string> const& cfg,
        startup_function_type startup = startup_function_type(),
        shutdown_function_type shutdown = shutdown_function_type(),
        hpx::runtime_mode mode = hpx::runtime_mode::default_);

    /// \brief Main non-blocking entry point for launching the HPX runtime system.
    ///
    /// This is the main, non-blocking entry point for any HPX application.
    /// This function (or one of its overloads below) should be called from the
    /// users `main()` function. It will set up the HPX runtime environment and
    /// schedule the function given by \p f as a HPX thread. It will return
    /// immediately after that. Use `hpx::wait` and `hpx::stop` to synchronize
    /// with the runtime system's execution. This overload will not call
    /// `hpx_main`.
    ///
    /// \param f            [in] The function to be scheduled as an HPX
    ///                     thread. Usually this function represents the main
    ///                     entry point of any HPX application. If \p f is
    ///                     `nullptr` the HPX runtime environment will be started
    ///                     without invoking \p f.
    /// \param desc_cmdline [in] This parameter may hold the description of
    ///                     additional command line arguments understood by the
    ///                     application. These options will be prepended to
    ///                     the default command line options understood by
    ///                     \a hpx::init (see description below).
    /// \param argc         [in] The number of command line arguments passed
    ///                     in \p argv. This is usually the unchanged value as
    ///                     passed by the operating system (to `main()`).
    /// \param argv         [in] The command line arguments for this
    ///                     application, usually that is the value as passed
    ///                     by the operating system (to `main()`).
    /// \param startup      [in] A function to be executed inside a HPX
    ///                     thread before \p f is called. If this parameter
    ///                     is not given no function will be executed.
    /// \param shutdown     [in] A function to be executed inside an HPX
    ///                     thread while hpx::finalize is executed. If this
    ///                     parameter is not given no function will be
    ///                     executed.
    /// \param mode         [in] The mode the created runtime environment
    ///                     should be initialized in. There has to be exactly
    ///                     one locality in each HPX application which is
    ///                     executed in console mode (\a hpx::runtime_mode::console),
    ///                     all other localities have to be run in worker mode
    ///                     (\a hpx::runtime_mode::worker). Normally this is
    ///                     set up automatically, but sometimes it is necessary
    ///                     to explicitly specify the mode.
    ///
    /// \returns            The function returns \a true if command line processing
    ///                     succeeded and the runtime system was started successfully.
    ///                     It will return \a false otherwise.
    ///
    /// \note               If the parameter \p mode is not given (defaulted),
    ///                     the created runtime system instance will be
    ///                     executed in console or worker mode depending on the
    ///                     command line arguments passed in `argc`/`argv`.
    ///                     Otherwise it will be executed as specified by the
    ///                     parameter\p mode.
    inline bool start(int (*f)(hpx::program_options::variables_map& vm),
        hpx::program_options::options_description const& desc_cmdline, int argc,
        char** argv, startup_function_type startup = startup_function_type(),
        shutdown_function_type shutdown = shutdown_function_type(),
        hpx::runtime_mode mode = hpx::runtime_mode::default_);

    /// \brief Main non-blocking entry point for launching the HPX runtime system.
    ///
    /// This is a simplified main, non-blocking entry point, which can be used
    /// to set up the runtime for an HPX application (the runtime system will be
    /// set up in console mode or worker mode depending on the command line
    /// settings). It will return immediately after that. Use `hpx::wait` and
    /// `hpx::stop` to synchronize with the runtime system's execution.
    ///
    /// In console mode it will execute the user supplied function `hpx_main`,
    /// in worker mode it will execute an empty `hpx_main`.
    ///
    /// \param desc_cmdline [in] This parameter may hold the description of
    ///                     additional command line arguments understood by the
    ///                     application. These options will be prepended to
    ///                     the default command line options understood by
    ///                     \a hpx::init (see description below).
    /// \param argc         [in] The number of command line arguments passed
    ///                     in \p argv. This is usually the unchanged value as
    ///                     passed by the operating system (to `main()`).
    /// \param argv         [in] The command line arguments for this
    ///                     application, usually that is the value as passed
    ///                     by the operating system (to `main()`).
    /// \param startup      [in] A function to be executed inside a HPX
    ///                     thread before \p f is called. If this parameter
    ///                     is not given no function will be executed.
    /// \param shutdown     [in] A function to be executed inside an HPX
    ///                     thread while hpx::finalize is executed. If this
    ///                     parameter is not given no function will be
    ///                     executed.
    /// \param mode         [in] The mode the created runtime environment
    ///                     should be initialized in. There has to be exactly
    ///                     one locality in each HPX application which is
    ///                     executed in console mode (\a hpx::runtime_mode::console),
    ///                     all other localities have to be run in worker mode
    ///                     (\a hpx::runtime_mode::worker). Normally this is
    ///                     set up automatically, but sometimes it is necessary
    ///                     to explicitly specify the mode.
    ///
    /// \returns            The function returns \a true if command line processing
    ///                     succeeded and the runtime system was started successfully.
    ///                     It will return \a false otherwise.
    ///
    /// \note               If the parameter \p mode is not given (defaulted),
    ///                     the created runtime system instance will be
    ///                     executed in console or worker mode depending on the
    ///                     command line arguments passed in `argc`/`argv`.
    ///                     Otherwise it will be executed as specified by the
    ///                     parameter\p mode.
    inline bool start(
        hpx::program_options::options_description const& desc_cmdline, int argc,
        char** argv, startup_function_type startup = startup_function_type(),
        shutdown_function_type shutdown = shutdown_function_type(),
        hpx::runtime_mode mode = hpx::runtime_mode::default_);

    /// \brief Main non-blocking entry point for launching the HPX runtime system.
    ///
    /// This is a simplified main, non-blocking entry point, which can be used
    /// to set up the runtime for an HPX application (the runtime system will
    /// be set up in console mode or worker mode depending on the command line
    /// settings). It will return immediately after that. Use `hpx::wait` and
    /// `hpx::stop` to synchronize with the runtime system's execution.
    ///
    /// In console mode it will execute the user supplied function `hpx_main`,
    /// in worker mode it will execute an empty `hpx_main`.
    ///
    /// \param desc_cmdline [in] This parameter may hold the description of
    ///                     additional command line arguments understood by the
    ///                     application. These options will be prepended to
    ///                     the default command line options understood by
    ///                     \a hpx::init (see description below).
    /// \param argc         [in] The number of command line arguments passed
    ///                     in \p argv. This is usually the unchanged value as
    ///                     passed by the operating system (to `main()`).
    /// \param argv         [in] The command line arguments for this
    ///                     application, usually that is the value as passed
    ///                     by the operating system (to `main()`).
    /// \param cfg          A list of configuration settings which will be added
    ///                     to the system configuration before the runtime
    ///                     instance is run. Each of the entries in this list
    ///                     must have the format of a fully defined key/value
    ///                     pair from an ini-file (for instance
    ///                     'hpx.component.enabled=1')
    /// \param startup      [in] A function to be executed inside a HPX
    ///                     thread before \p f is called. If this parameter
    ///                     is not given no function will be executed.
    /// \param shutdown     [in] A function to be executed inside an HPX
    ///                     thread while hpx::finalize is executed. If this
    ///                     parameter is not given no function will be
    ///                     executed.
    /// \param mode         [in] The mode the created runtime environment
    ///                     should be initialized in. There has to be exactly
    ///                     one locality in each HPX application which is
    ///                     executed in console mode (\a hpx::runtime_mode::console),
    ///                     all other localities have to be run in worker mode
    ///                     (\a hpx::runtime_mode::worker). Normally this is
    ///                     set up automatically, but sometimes it is necessary
    ///                     to explicitly specify the mode.
    ///
    /// \returns            The function returns \a true if command line processing
    ///                     succeeded and the runtime system was started successfully.
    ///                     It will return \a false otherwise.
    ///
    /// \note               If the parameter \p mode is not given (defaulted),
    ///                     the created runtime system instance will be
    ///                     executed in console or worker mode depending on the
    ///                     command line arguments passed in `argc`/`argv`.
    ///                     Otherwise it will be executed as specified by the
    ///                     parameter\p mode.
    inline bool start(
        hpx::program_options::options_description const& desc_cmdline, int argc,
        char** argv, std::vector<std::string> const& cfg,
        startup_function_type startup = startup_function_type(),
        shutdown_function_type shutdown = shutdown_function_type(),
        hpx::runtime_mode mode = hpx::runtime_mode::default_);

    /// \brief Main non-blocking entry point for launching the HPX runtime system.
    ///
    /// This is a simplified main, non-blocking entry point, which can be used
    /// to set up the runtime for an HPX application (the runtime system will
    /// be set up in console mode or worker mode depending on the command line
    /// settings). It will return immediately after that. Use `hpx::wait` and
    /// `hpx::stop` to synchronize with the runtime system's execution.
    ///
    /// In console mode it will execute the user supplied function `hpx_main`,
    /// in worker mode it will execute an empty `hpx_main`.
    ///
    /// \param argc         [in] The number of command line arguments passed
    ///                     in \p argv. This is usually the unchanged value as
    ///                     passed by the operating system (to `main()`).
    /// \param argv         [in] The command line arguments for this
    ///                     application, usually that is the value as passed
    ///                     by the operating system (to `main()`).
    /// \param cfg          A list of configuration settings which will be added
    ///                     to the system configuration before the runtime
    ///                     instance is run. Each of the entries in this list
    ///                     must have the format of a fully defined key/value
    ///                     pair from an ini-file (for instance
    ///                     'hpx.component.enabled=1')
    /// \param mode         [in] The mode the created runtime environment
    ///                     should be initialized in. There has to be exactly
    ///                     one locality in each HPX application which is
    ///                     executed in console mode (\a hpx::runtime_mode::console),
    ///                     all other localities have to be run in worker mode
    ///                     (\a hpx::runtime_mode::worker). Normally this is
    ///                     set up automatically, but sometimes it is necessary
    ///                     to explicitly specify the mode.
    ///
    /// \returns            The function returns \a true if command line processing
    ///                     succeeded and the runtime system was started successfully.
    ///                     It will return \a false otherwise.
    ///
    /// \note               If the parameter \p mode is runtime_mode::default_,
    ///                     the created runtime system instance will be
    ///                     executed in console or worker mode depending on the
    ///                     command line arguments passed in `argc`/`argv`.
    ///                     Otherwise it will be executed as specified by the
    ///                     parameter\p mode.
    inline bool start(int argc, char** argv,
        std::vector<std::string> const& cfg,
        hpx::runtime_mode mode = hpx::runtime_mode::default_);

    /// \brief Main non-blocking entry point for launching the HPX runtime system.
    ///
    /// This is a simplified main, non-blocking entry point, which can be used
    /// to set up the runtime for an HPX application (the runtime system will
    /// be set up in console mode or worker mode depending on the command line
    /// settings). It will return immediately after that. Use `hpx::wait` and
    /// `hpx::stop` to synchronize with the runtime system's execution.
    ///
    /// In console mode it will execute the user supplied function `hpx_main`,
    /// in worker mode it will execute an empty `hpx_main`.
    ///
    /// \param desc_cmdline [in] This parameter may hold the description of
    ///                     additional command line arguments understood by the
    ///                     application. These options will be prepended to
    ///                     the default command line options understood by
    ///                     \a hpx::init (see description below).
    /// \param argc         [in] The number of command line arguments passed
    ///                     in \p argv. This is usually the unchanged value as
    ///                     passed by the operating system (to `main()`).
    /// \param argv         [in] The command line arguments for this
    ///                     application, usually that is the value as passed
    ///                     by the operating system (to `main()`).
    /// \param mode         [in] The mode the created runtime environment
    ///                     should be initialized in. There has to be exactly
    ///                     one locality in each HPX application which is
    ///                     executed in console mode (\a hpx::runtime_mode::console),
    ///                     all other localities have to be run in worker mode
    ///                     (\a hpx::runtime_mode::worker). Normally this is
    ///                     set up automatically, but sometimes it is necessary
    ///                     to explicitly specify the mode.
    ///
    /// \returns            The function returns \a true if command line processing
    ///                     succeeded and the runtime system was started successfully.
    ///                     It will return \a false otherwise.
    ///
    /// \note               If the parameter \p mode is runtime_mode::default_,
    ///                     the created runtime system instance will be
    ///                     executed in console or worker mode depending on the
    ///                     command line arguments passed in `argc`/`argv`.
    ///                     Otherwise it will be executed as specified by the
    ///                     parameter\p mode.
    inline bool start(
        hpx::program_options::options_description const& desc_cmdline, int argc,
        char** argv, hpx::runtime_mode mode);

    /// \brief Main non-blocking entry point for launching the HPX runtime system.
    ///
    /// This is a simplified main, non-blocking entry point, which can be used
    /// to set up the runtime for an HPX application (the runtime system will
    /// be set up in console mode or worker mode depending on the command line
    /// settings). It will return immediately after that. Use `hpx::wait` and
    /// `hpx::stop` to synchronize with the runtime system's execution.
    ///
    /// In console mode it will execute the user supplied function `hpx_main`,
    /// in worker mode it will execute an empty `hpx_main`.
    ///
    /// \param desc_cmdline [in] This parameter may hold the description of
    ///                     additional command line arguments understood by the
    ///                     application. These options will be prepended to
    ///                     the default command line options understood by
    ///                     \a hpx::init (see description below).
    /// \param argc         [in] The number of command line arguments passed
    ///                     in \p argv. This is usually the unchanged value as
    ///                     passed by the operating system (to `main()`).
    /// \param argv         [in] The command line arguments for this
    ///                     application, usually that is the value as passed
    ///                     by the operating system (to `main()`).
    /// \param cfg          A list of configuration settings which will be added
    ///                     to the system configuration before the runtime
    ///                     instance is run. Each of the entries in this list
    ///                     must have the format of a fully defined key/value
    ///                     pair from an ini-file (for instance
    ///                     'hpx.component.enabled=1')
    /// \param mode         [in] The mode the created runtime environment
    ///                     should be initialized in. There has to be exactly
    ///                     one locality in each HPX application which is
    ///                     executed in console mode (\a hpx::runtime_mode::console),
    ///                     all other localities have to be run in worker mode
    ///                     (\a hpx::runtime_mode::worker). Normally this is
    ///                     set up automatically, but sometimes it is necessary
    ///                     to explicitly specify the mode.
    ///
    /// \returns            The function returns \a true if command line processing
    ///                     succeeded and the runtime system was started successfully.
    ///                     It will return \a false otherwise.
    ///
    /// \note               If the parameter \p mode is runtime_mode::default_,
    ///                     the created runtime system instance will be
    ///                     executed in console or worker mode depending on the
    ///                     command line arguments passed in `argc`/`argv`.
    ///                     Otherwise it will be executed as specified by the
    ///                     parameter\p mode.
    inline bool start(
        hpx::program_options::options_description const& desc_cmdline, int argc,
        char** argv, std::vector<std::string> const& cfg,
        hpx::runtime_mode mode);

    /// \brief Main non-blocking entry point for launching the HPX runtime system.
    ///
    /// This is a simplified main, non-blocking entry point, which can be used
    /// to set up the runtime for an HPX application (the runtime system will
    /// be set up in console mode or worker mode depending on the command line
    /// settings). It will return immediately after that. Use `hpx::wait` and
    /// `hpx::stop` to synchronize with the runtime system's execution.
    ///
    /// \param app_name     [in] The name of the application.
    /// \param argc         [in] The number of command line arguments passed
    ///                     in \p argv. This is usually the unchanged value as
    ///                     passed by the operating system (to `main()`).
    /// \param argv         [in] The command line arguments for this
    ///                     application, usually that is the value as passed
    ///                     by the operating system (to `main()`).
    /// \param mode         [in] The mode the created runtime environment
    ///                     should be initialized in. There has to be exactly
    ///                     one locality in each HPX application which is
    ///                     executed in console mode (\a hpx::runtime_mode::console),
    ///                     all other localities have to be run in worker mode
    ///                     (\a hpx::runtime_mode::worker). Normally this is
    ///                     set up automatically, but sometimes it is necessary
    ///                     to explicitly specify the mode.
    ///
    /// \returns            The function returns \a true if command line processing
    ///                     succeeded and the runtime system was started successfully.
    ///                     It will return \a false otherwise.
    ///
    /// \note               The created runtime system instance will be
    ///                     executed in console or worker mode depending on the
    ///                     command line arguments passed in `argc`/`argv`.
    inline bool start(std::string const& app_name, int argc = 0,
        char** argv = nullptr,
        hpx::runtime_mode mode = hpx::runtime_mode::default_);

    /// \brief Main non-blocking entry point for launching the HPX runtime system.
    ///
    /// This is a simplified main, non-blocking entry point, which can be used
    /// to set up the runtime for an HPX application (the runtime system will
    /// be set up in console mode or worker mode depending on the command line
    /// settings). It will return immediately after that. Use `hpx::wait` and
    /// `hpx::stop` to synchronize with the runtime system's execution.
    ///
    /// \param cfg          A list of configuration settings which will be added
    ///                     to the system configuration before the runtime
    ///                     instance is run. Each of the entries in this list
    ///                     must have the format of a fully defined key/value
    ///                     pair from an ini-file (for instance
    ///                     'hpx.component.enabled=1')
    /// \param mode         [in] The mode the created runtime environment
    ///                     should be initialized in. There has to be exactly
    ///                     one locality in each HPX application which is
    ///                     executed in console mode (\a hpx::runtime_mode::console),
    ///                     all other localities have to be run in worker mode
    ///                     (\a hpx::runtime_mode::worker). Normally this is
    ///                     set up automatically, but sometimes it is necessary
    ///                     to explicitly specify the mode.
    ///
    /// \returns            The function returns \a true if command line processing
    ///                     succeeded and the runtime system was started successfully.
    ///                     It will return \a false otherwise.
    ///
    /// \note               The created runtime system instance will be
    ///                     executed in console or worker mode depending on the
    ///                     command line arguments passed in `argc`/`argv`. If not
    ///                     command line arguments are passed, console mode is
    ///                     assumed.
    ///
    /// \note               If no command line arguments are passed the HPX
    ///                     runtime system will not support any of the default
    ///                     command line options as described in the section
    ///                     'HPX Command Line Options'.
    inline bool start(std::vector<std::string> const& cfg,
        hpx::runtime_mode mode = hpx::runtime_mode::default_);

    /// \brief Main non-blocking entry point for launching the HPX runtime system.
    ///
    /// This is a simplified main, non-blocking entry point, which can be used
    /// to set up the runtime for an HPX application (the runtime system will
    /// be set up in console mode or worker mode depending on the command line
    /// settings). It will return immediately after that. Use `hpx::wait` and
    /// `hpx::stop` to synchronize with the runtime system's execution. This
    /// overload will schedule the function given by \p f as a HPX thread. It
    /// will not call `hpx_main`.
    ///
    /// \param f            [in] The function to be scheduled as an HPX
    ///                     thread. Usually this function represents the main
    ///                     entry point of any HPX application.
    /// \param app_name     [in] The name of the application.
    /// \param argc         [in] The number of command line arguments passed
    ///                     in \p argv. This is usually the unchanged value as
    ///                     passed by the operating system (to `main()`).
    /// \param argv         [in] The command line arguments for this
    ///                     application, usually that is the value as passed
    ///                     by the operating system (to `main()`).
    /// \param mode         [in] The mode the created runtime environment
    ///                     should be initialized in. There has to be exactly
    ///                     one locality in each HPX application which is
    ///                     executed in console mode (\a hpx::runtime_mode::console),
    ///                     all other localities have to be run in worker mode
    ///                     (\a hpx::runtime_mode::worker). Normally this is
    ///                     set up automatically, but sometimes it is necessary
    ///                     to explicitly specify the mode.
    ///
    /// \returns            The function returns \a true if command line processing
    ///                     succeeded and the runtime system was started successfully.
    ///                     It will return \a false otherwise.
    ///
    /// \note               The created runtime system instance will be
    ///                     executed in console or worker mode depending on the
    ///                     command line arguments passed in `argc`/`argv`.
    inline bool start(int (*f)(hpx::program_options::variables_map& vm),
        std::string const& app_name, int argc, char** argv,
        hpx::runtime_mode mode = hpx::runtime_mode::default_);

    /// \brief Main non-blocking entry point for launching the HPX runtime system.
    ///
    /// This is a simplified main, non-blocking entry point, which can be used
    /// to set up the runtime for an HPX application (the runtime system will
    /// be set up in console mode or worker mode depending on the command line
    /// settings). It will return immediately after that. Use `hpx::wait` and
    /// `hpx::stop` to synchronize with the runtime system's execution. This
    /// overload will schedule the function given by \p f as a HPX thread. It
    /// will not call `hpx_main`.
    ///
    /// \param f            [in] The function to be scheduled as an HPX
    ///                     thread. Usually this function represents the main
    ///                     entry point of any HPX application. If \p f is
    ///                     `nullptr` the HPX runtime environment will be started
    ///                     without invoking \p f.
    /// \param app_name     [in] The name of the application.
    /// \param argc         [in] The number of command line arguments passed
    ///                     in \p argv. This is usually the unchanged value as
    ///                     passed by the operating system (to `main()`).
    /// \param argv         [in] The command line arguments for this
    ///                     application, usually that is the value as passed
    ///                     by the operating system (to `main()`).
    /// \param mode         [in] The mode the created runtime environment
    ///                     should be initialized in. There has to be exactly
    ///                     one locality in each HPX application which is
    ///                     executed in console mode (\a hpx::runtime_mode::console),
    ///                     all other localities have to be run in worker mode
    ///                     (\a hpx::runtime_mode::worker). Normally this is
    ///                     set up automatically, but sometimes it is necessary
    ///                     to explicitly specify the mode.
    ///
    /// \returns            The function returns \a true if command line processing
    ///                     succeeded and the runtime system was started successfully.
    ///                     It will return \a false otherwise.
    ///
    /// \note               The created runtime system instance will be
    ///                     executed in console or worker mode depending on the
    ///                     command line arguments passed in `argc`/`argv`.
    inline bool start(util::function_nonser<int(int, char**)> const& f,
        std::string const& app_name, int argc, char** argv,
        hpx::runtime_mode mode = hpx::runtime_mode::default_);

    /// \brief Main non-blocking entry point for launching the HPX runtime system.
    ///
    /// This is a simplified main, non-blocking entry point, which can be used
    /// to set up the runtime for an HPX application (the runtime system will
    /// be set up in console mode or worker mode depending on the command line
    /// settings). It will return immediately after that. Use `hpx::wait` and
    /// `hpx::stop` to synchronize with the runtime system's execution. This
    /// overload will schedule the function given by \p f as a HPX thread. It
    /// will not call `hpx_main`.
    ///
    /// \param f            [in] The function to be scheduled as an HPX
    ///                     thread. Usually this function represents the main
    ///                     entry point of any HPX application. If \p f is
    ///                     `nullptr` the HPX runtime environment will be started
    ///                     without invoking \p f.
    /// \param argc         [in] The number of command line arguments passed
    ///                     in \p argv. This is usually the unchanged value as
    ///                     passed by the operating system (to `main()`).
    /// \param argv         [in] The command line arguments for this
    ///                     application, usually that is the value as passed
    ///                     by the operating system (to `main()`).
    /// \param cfg          A list of configuration settings which will be added
    ///                     to the system configuration before the runtime
    ///                     instance is run. Each of the entries in this list
    ///                     must have the format of a fully defined key/value
    ///                     pair from an ini-file (for instance
    ///                     'hpx.component.enabled=1')
    /// \param mode         [in] The mode the created runtime environment
    ///                     should be initialized in. There has to be exactly
    ///                     one locality in each HPX application which is
    ///                     executed in console mode (\a hpx::runtime_mode::console),
    ///                     all other localities have to be run in worker mode
    ///                     (\a hpx::runtime_mode::worker). Normally this is
    ///                     set up automatically, but sometimes it is necessary
    ///                     to explicitly specify the mode.
    ///
    /// \returns            The function returns \a true if command line processing
    ///                     succeeded and the runtime system was started successfully.
    ///                     It will return \a false otherwise.
    ///
    /// \note               The created runtime system instance will be
    ///                     executed in console or worker mode depending on the
    ///                     command line arguments passed in `argc`/`argv`.
    inline bool start(util::function_nonser<int(int, char**)> const& f,
        int argc, char** argv, std::vector<std::string> const& cfg,
        hpx::runtime_mode mode = hpx::runtime_mode::default_);

    /// \brief Main non-blocking entry point for launching the HPX runtime system.
    ///
    /// This is a simplified main, non-blocking entry point, which can be used
    /// to set up the runtime for an HPX application (the runtime system will
    /// be set up in console mode or worker mode depending on the command line
    /// settings). It will return immediately after that. Use `hpx::wait` and
    /// `hpx::stop` to synchronize with the runtime system's execution. This
    /// overload will schedule the function given by \p f as a HPX thread. It
    /// will not call `hpx_main`.
    ///
    /// \param f            [in] The function to be scheduled as an HPX
    ///                     thread. Usually this function represents the main
    ///                     entry point of any HPX application. If \p f is
    ///                     `nullptr` the HPX runtime environment will be started
    ///                     without invoking \p f.
    /// \param cfg          A list of configuration settings which will be added
    ///                     to the system configuration before the runtime
    ///                     instance is run. Each of the entries in this list
    ///                     must have the format of a fully defined key/value
    ///                     pair from an ini-file (for instance
    ///                     'hpx.component.enabled=1')
    /// \param mode         [in] The mode the created runtime environment
    ///                     should be initialized in. There has to be exactly
    ///                     one locality in each HPX application which is
    ///                     executed in console mode (\a hpx::runtime_mode::console),
    ///                     all other localities have to be run in worker mode
    ///                     (\a hpx::runtime_mode::worker). Normally this is
    ///                     set up automatically, but sometimes it is necessary
    ///                     to explicitly specify the mode.
    ///
    /// \returns            The function returns \a true if command line processing
    ///                     succeeded and the runtime system was started successfully.
    ///                     It will return \a false otherwise.
    ///
    /// \note               The created runtime system instance will be
    ///                     executed in console or worker mode depending on the
    ///                     configuration passed in `cfg`.
    inline bool start(util::function_nonser<int(int, char**)> const& f,
        std::vector<std::string> const& cfg,
        hpx::runtime_mode mode = hpx::runtime_mode::default_);

    /// \cond NOINTERNAL
    inline bool start(std::nullptr_t f, std::string const& app_name, int argc,
        char** argv, hpx::runtime_mode mode = hpx::runtime_mode::default_);

    inline bool start(std::nullptr_t f, int argc, char** argv,
        std::vector<std::string> const& cfg,
        hpx::runtime_mode mode = hpx::runtime_mode::default_);

    inline bool start(std::nullptr_t f, std::vector<std::string> const& cfg,
        hpx::runtime_mode mode = hpx::runtime_mode::default_);
/// \endcond
#endif
}    // namespace hpx

#if !defined(DOXYGEN)
///////////////////////////////////////////////////////////////////////////////
// Pull in the implementation of the inlined hpx::init functions if we're not
// compiling the core HPX library.
#if !defined(HPX_EXPORTS)
#include <hpx/hpx_start_impl.hpp>
#endif
#endif
