cmake_minimum_required(VERSION 3.22)

# -----------------------------------------------------------------------------
# Project: SparseH5Access plugin testing
# -----------------------------------------------------------------------------
set(SPARSEH5ACCESS_TESTS "SparseH5AccessTests")

# Setup of test build depends on setup of parent project, the plugin itself

PROJECT(${SPARSEH5ACCESS_TESTS}
        DESCRIPTION "Test for the SparseH5Access plugin"
        LANGUAGES CXX)

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------

find_package(Catch2 CONFIG REQUIRED)

# -----------------------------------------------------------------------------
# Source files
# -----------------------------------------------------------------------------

set(SPARSEH5ACCESS_TESTS_FUNCTIONS
    test_main.cpp
	test_utils.h
)

set(SPARSEH5ACCESS_PLUGIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src)

set(SPARSEH5ACCESS_MAIN_FUNCTIONS
    ${SPARSEH5ACCESS_PLUGIN_DIR}/H5Utils.h
    ${SPARSEH5ACCESS_PLUGIN_DIR}/H5Utils.cpp
)

set(SPARSEH5ACCESS_TEST_SOURCES
    ${SPARSEH5ACCESS_TESTS_FUNCTIONS}
    ${SPARSEH5ACCESS_MAIN_FUNCTIONS}
)

source_group( SparseH5AccessTests FILES ${SPARSEH5ACCESS_TEST_SOURCES})

# -----------------------------------------------------------------------------
# CMake Target
# -----------------------------------------------------------------------------

add_executable(${SPARSEH5ACCESS_TESTS} ${SPARSEH5ACCESS_TEST_SOURCES})

# -----------------------------------------------------------------------------
# Target include directories
# -----------------------------------------------------------------------------

target_include_directories(${SPARSEH5ACCESS_TESTS} PRIVATE "${SPARSEH5ACCESS_PLUGIN_DIR}")
target_include_directories(${SPARSEH5ACCESS_TESTS} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

# -----------------------------------------------------------------------------
# Target link directories
# -----------------------------------------------------------------------------
target_link_libraries(${SPARSEH5ACCESS_TESTS} PRIVATE Catch2::Catch2WithMain)
target_link_libraries(${SPARSEH5ACCESS_TESTS} PRIVATE hdf5::hdf5_cpp-static hdf5::hdf5_hl_cpp-static)

if(${MV_SH5A_USE_OPENMP} AND OpenMP_CXX_FOUND)
	message(STATUS "Link ${SPARSEH5ACCESS_TESTS} to OpenMP")
	target_link_libraries(${SPARSEH5ACCESS_TESTS} PRIVATE OpenMP::OpenMP_CXX)
endif()

# -----------------------------------------------------------------------------
# Target properties
# -----------------------------------------------------------------------------

# Request C++20
target_compile_features(${SPARSEH5ACCESS_TESTS} PRIVATE cxx_std_20)
target_compile_features(${SPARSEH5ACCESS_TESTS} PRIVATE c_std_17)

# Instruction sets
mv_check_and_set_AVX(${SPARSEH5ACCESS_TESTS} ${MV_UMAP_USE_AVX})
