# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindFoo
-------

Finds the Foo library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``Foo::Foo``
  The Foo library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``Foo_FOUND``
  True if the system has the Foo library.
``Foo_VERSION``
  The version of the Foo library which was found.
``Foo_INCLUDE_DIRS``
  Include directories needed to use Foo.
``Foo_LIBRARIES``
  Libraries needed to link to Foo.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Foo_INCLUDE_DIR``
  The directory containing ``foo.h``.
``Foo_LIBRARY``
  The path to the Foo library.

#]=======================================================================]

set(PROJECT_ROOT_DIR ${CMAKE_SOURCE_DIR})

if(CMAKE_CL_64)
    set(arch x86_64)
else()
    set(arch x86)
endif()

find_path(avcodec_INCLUDE_DIR
    NAMES avcodec.h
    PATHS ${PROJECT_ROOT_DIR}/include/libavcodec
)
message(STATUS "avcodec_INCLUDE_DIR: ${avcodec_INCLUDE_DIR}")

find_library(avcodec_LIBRARY
    NAMES avcodec
    PATHS ${PROJECT_ROOT_DIR}/lib
)
# find_library(avcodec_LIBRARY_DEBUG
#     NAMES avcodec
#     PATHS ${PROJECT_ROOT_DIR}/lib
# )

# include(SelectLibraryConfigurations)
# select_library_configurations(avcodec)
message(STATUS "avcodec_LIBRARY: ${avcodec_LIBRARY}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(avcodec
    FOUND_VAR avcodec_FOUND
    REQUIRED_VARS
    avcodec_LIBRARY
    avcodec_INCLUDE_DIR
)

if(avcodec_FOUND)
    if(NOT TARGET avcodec::avcodec)
        add_library(avcodec::avcodec UNKNOWN IMPORTED)
    endif()

    if(avcodec_LIBRARY)
        set_property(TARGET avcodec::avcodec APPEND PROPERTY
            IMPORTED_CONFIGURATIONS RELEASE
        )
        set_target_properties(avcodec::avcodec PROPERTIES
            IMPORTED_LOCATION "${avcodec_LIBRARY}"
        )
    endif()
    # if(avcodec_LIBRARY_RELEASE)
    #     set_property(TARGET avcodec::avcodec APPEND PROPERTY
    #         IMPORTED_CONFIGURATIONS RELEASE
    #     )
    #     set_target_properties(avcodec::avcodec PROPERTIES
    #         IMPORTED_LOCATION_RELEASE "${avcodec_LIBRARY_RELEASE}"
    #     )
    # endif()

    # if(avcodec_LIBRARY_DEBUG)
    #     set_property(TARGET avcodec::avcodec APPEND PROPERTY
    #         IMPORTED_CONFIGURATIONS DEBUG
    #     )
    #     set_target_properties(avcodec::avcodec PROPERTIES
    #         IMPORTED_LOCATION_DEBUG "${avcodec_LIBRARY_DEBUG}"
    #     )
    # endif()

    set_target_properties(avcodec::avcodec PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${avcodec_INCLUDE_DIR}/.."
    )
endif()

mark_as_advanced(
    avcodec_INCLUDE_DIR
    avcodec_LIBRARY
)


if(avcodec_FOUND)
    set(avcodec_LIBRARIES ${avcodec_LIBRARY})
    set(avcodec_INCLUDE_DIRS ${avcodec_INCLUDE_DIR})
    set(avcodec_LIB_TARGET avcodec::avcodec)
endif()
