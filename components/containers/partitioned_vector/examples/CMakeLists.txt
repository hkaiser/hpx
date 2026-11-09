# Copyright (c) 2019 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(HPX_WITH_EXAMPLES)
  add_hpx_pseudo_target(examples.components.partitioned_vector)
  add_hpx_pseudo_dependencies(
    examples.components examples.components.partitioned_vector
  )
  if(HPX_WITH_TESTS AND HPX_WITH_TESTS_EXAMPLES)
    add_hpx_pseudo_target(tests.examples.components.partitioned_vector)
    add_hpx_pseudo_dependencies(
      tests.examples.components tests.examples.components.partitioned_vector
    )
  endif()

  # a partitioned_vector whose partitions are thrust::device_vector
  if(HPX_WITH_CUDA AND HPX_WITH_THRUST)
    set(example_programs partitioned_device_vector)
    set(partitioned_device_vector_SOURCES example.cu)
    set(partitioned_device_vector_FLAGS COMPONENT_DEPENDENCIES
                                        partitioned_vector
    )
    set(partitioned_device_vector_PARAMETERS THREADS_PER_LOCALITY 4)

    foreach(example_program ${example_programs})
      set(sources ${${example_program}_SOURCES})
      source_group("Source Files" FILES ${sources})

      add_hpx_executable(
        ${example_program} INTERNAL_FLAGS
        SOURCES ${sources} ${${example_program}_FLAGS}
        EXCLUDE_FROM_ALL
        HPX_PREFIX ${HPX_BUILD_PREFIX}
        FOLDER "Examples/Components/PartitionedVector"
      )

      add_hpx_example_target_dependencies(
        "components.partitioned_vector" ${example_program}
      )

      if(HPX_WITH_TESTS AND HPX_WITH_TESTS_EXAMPLES)
        add_hpx_example_test(
          "components.partitioned_vector" ${example_program}
          ${${example_program}_PARAMETERS}
        )
      endif()
    endforeach()
  endif()
endif()
