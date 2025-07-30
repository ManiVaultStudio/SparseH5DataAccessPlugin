cmake_minimum_required(VERSION 3.22)

option(MV_SH5A_USE_OPENMP "Use OpenMP - by default ON" ON)
option(MV_SH5A_USE_AVX "Use AVX if available - by default OFF" OFF)

# -----------------------------------------------------------------------------
# vcpkg Options
# -----------------------------------------------------------------------------

if(WIN32)
    set(VCPKG_HOST_TRIPLET "x64-windows-static-md" CACHE STRING "")
    set(VCPKG_TARGET_TRIPLET "x64-windows-static-md" CACHE STRING "")
    set(VCPKG_LIBRARY_LINKAGE "static" CACHE STRING "")
endif()

# -----------------------------------------------------------------------------
# SparseH5Access Analysis Plugin
# -----------------------------------------------------------------------------
set(SPARSEH5ACCESS "SparseH5Access")
PROJECT(${SPARSEH5ACCESS} 
        DESCRIPTION "A ManiVault plugin that access sparse H5 data from disk"
        LANGUAGES CXX)

# -----------------------------------------------------------------------------
# CMake Options
# -----------------------------------------------------------------------------
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DWIN32 /EHsc /MP /permissive- /Zc:__cplusplus")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MD")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
endif()

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
find_package(Qt6 COMPONENTS Widgets WebEngineWidgets REQUIRED)
find_package(ManiVault COMPONENTS Core PointData CONFIG QUIET)

find_package(hdf5 CONFIG REQUIRED COMPONENTS CXX HL)
message(STATUS "Found HDF5 with version ${hdf5_VERSION}")

if(MV_SH5A_USE_OPENMP)
    find_package(OpenMP)

    if(OpenMP_CXX_FOUND)
        message(STATUS "Found OpenMP")
    endif()
endif()


# -----------------------------------------------------------------------------
# Source files
# -----------------------------------------------------------------------------
# Define the plugin sources
set(SPARSEH5ACCESS_PLUGIN
    src/SparseH5AccessPlugin.h
    src/SparseH5AccessPlugin.cpp
    src/H5Utils.h
    src/H5Utils.cpp
)

set(SPARSEH5ACCESS_SETTINGS
    src/SettingsAction.h
    src/SettingsAction.cpp
)

set(SPARSEH5ACCESS_AUX
    PluginInfo.json
)

set(SPARSEH5ACCESS_SOURCES
    ${SPARSEH5ACCESS_PLUGIN}
    ${SPARSEH5ACCESS_SETTINGS}
)

source_group(Plugin FILES ${SPARSEH5ACCESS_PLUGIN})
source_group(Settings FILES ${SPARSEH5ACCESS_SETTINGS})
source_group(Aux FILES ${SPARSEH5ACCESS_AUX})

# -----------------------------------------------------------------------------
# CMake Target
# -----------------------------------------------------------------------------
# Create dynamic library for the plugin
add_library(${SPARSEH5ACCESS} SHARED ${SPARSEH5ACCESS_SOURCES} ${SPARSEH5ACCESS_AUX})

# -----------------------------------------------------------------------------
# Target include directories
# -----------------------------------------------------------------------------
# Include ManiVault headers, including system data plugins
target_include_directories(${SPARSEH5ACCESS} PRIVATE "${ManiVault_INCLUDE_DIR}")

# Inlcude own sources for cross-platform include consitency
target_include_directories(${SPARSEH5ACCESS} PRIVATE "${${CMAKE_CURRENT_SOURCE_DIR}/src}")

# -----------------------------------------------------------------------------
# Target properties
# -----------------------------------------------------------------------------
# Request C++20
target_compile_features(${SPARSEH5ACCESS} PRIVATE cxx_std_20)

# Instruction sets
mv_check_and_set_AVX(${SPARSEH5ACCESS} ${MV_SH5A_USE_AVX})

# -----------------------------------------------------------------------------
# Target library linking
# -----------------------------------------------------------------------------
target_link_libraries(${SPARSEH5ACCESS} PRIVATE Qt6::Widgets)
target_link_libraries(${SPARSEH5ACCESS} PRIVATE Qt6::WebEngineWidgets)

target_link_libraries(${SPARSEH5ACCESS} PRIVATE ManiVault::Core)
target_link_libraries(${SPARSEH5ACCESS} PRIVATE ManiVault::PointData)

target_link_libraries(${SPARSEH5ACCESS} PRIVATE hdf5::hdf5_cpp-static hdf5::hdf5_hl_cpp-static)

if(${MV_SH5A_USE_OPENMP} AND OpenMP_CXX_FOUND)
    target_link_libraries(${SPARSEH5ACCESS} PRIVATE OpenMP::OpenMP_CXX)
endif()

# -----------------------------------------------------------------------------
# Target installation
# -----------------------------------------------------------------------------
# Install the shared plugin library to the "Plugins" folder in the ManiVault install directory
install(TARGETS ${SPARSEH5ACCESS}
    RUNTIME DESTINATION Plugins COMPONENT PLUGIN # Windows .dll
    LIBRARY DESTINATION Plugins COMPONENT PLUGIN # Linux/Mac .so
)

add_custom_command(TARGET ${SPARSEH5ACCESS} POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
        --install ${CMAKE_CURRENT_BINARY_DIR}
        --config $<CONFIGURATION>
        --component PLUGIN
        --prefix ${ManiVault_INSTALL_DIR}/$<CONFIGURATION>
)

mv_handle_plugin_config(${SPARSEH5ACCESS})

# -----------------------------------------------------------------------------
# Miscellaneous
# -----------------------------------------------------------------------------
# Automatically set the debug environment (command + working directory) for MSVC
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set_property(TARGET ${SPARSEH5ACCESS} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<IF:$<CONFIG:DEBUG>,${ManiVault_INSTALL_DIR}/Debug,$<IF:$<CONFIG:RELWITHDEBINFO>,${ManiVault_INSTALL_DIR}/RelWithDebInfo,${ManiVault_INSTALL_DIR}/Release>>)
    set_property(TARGET ${SPARSEH5ACCESS} PROPERTY VS_DEBUGGER_COMMAND $<IF:$<CONFIG:DEBUG>,"${ManiVault_INSTALL_DIR}/Debug/ManiVault Studio.exe",$<IF:$<CONFIG:RELWITHDEBINFO>,"${ManiVault_INSTALL_DIR}/RelWithDebInfo/ManiVault Studio.exe","${ManiVault_INSTALL_DIR}/Release/ManiVault Studio.exe">>)
endif()
