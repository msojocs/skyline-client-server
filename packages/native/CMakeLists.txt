cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0042 NEW)
# cmake_policy(SET CMP0167 NEW)

project("module_skyline")

set(SKYLINE_COMPILER_MACHINE "")
if(CMAKE_CXX_COMPILER)
    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} -dumpmachine
        OUTPUT_VARIABLE SKYLINE_COMPILER_MACHINE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()
string(TOLOWER "${SKYLINE_COMPILER_MACHINE}" SKYLINE_COMPILER_MACHINE_LOWER)

set(SKYLINE_TARGET_WINDOWS OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows" OR WIN32 OR SKYLINE_COMPILER_MACHINE_LOWER MATCHES "mingw|windows")
    set(SKYLINE_TARGET_WINDOWS ON)
endif()

set(SKYLINE_USING_MINGW OFF)
if(MINGW OR SKYLINE_COMPILER_MACHINE_LOWER MATCHES "mingw")
    set(SKYLINE_USING_MINGW ON)
endif()

# Improve dependency scanning and header file tracking
set(CMAKE_DEPENDS_IN_PROJECT_ONLY OFF)
set(CMAKE_CXX_STANDARD 17)
# Enable header dependency scanning 
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /showIncludes")
endif()

# 性能优化标志
if(MSVC)
    # MSVC优化标志
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2 /Oi /Ot /GL /Qpar")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /favor:blend /arch:AVX2")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG")
else()
    # GCC/Clang优化标志
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -flto")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -flto")
endif()

set(VCPKG_ROOT $ENV{VCPKG_ROOT})
message("VCPKG_ROOT: " ${VCPKG_ROOT})
message("SKYLINE_COMPILER_MACHINE: ${SKYLINE_COMPILER_MACHINE}")
message("SKYLINE_TARGET_WINDOWS: ${SKYLINE_TARGET_WINDOWS}")
message("SKYLINE_USING_MINGW: ${SKYLINE_USING_MINGW}")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(PKG_TOOL pnpm)
set(TARGET_ARCH x64)
if(MSVC)
    execute_process(COMMAND
        chcp 65001
    )
    set(_skyline_default_triplet x64-windows-static)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
elseif(SKYLINE_USING_MINGW)
    # MinGW cross-compilation for Windows
    set(_skyline_default_triplet x64-mingw-static)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_REENTRANT")
else()
    # Linux specific settings
    set(_skyline_default_triplet x64-linux)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_REENTRANT")
endif()

if(NOT DEFINED VCPKG_TARGET_TRIPLET)
    set(VCPKG_TARGET_TRIPLET "${_skyline_default_triplet}" CACHE STRING "Vcpkg target triplet" FORCE)
elseif(SKYLINE_USING_MINGW AND VCPKG_TARGET_TRIPLET STREQUAL "x64-linux")
    message(WARNING "Detected MinGW compiler but cached VCPKG_TARGET_TRIPLET=x64-linux. Auto-correcting to x64-mingw-static.")
    set(VCPKG_TARGET_TRIPLET x64-mingw-static CACHE STRING "Vcpkg target triplet" FORCE)
elseif(NOT SKYLINE_TARGET_WINDOWS AND VCPKG_TARGET_TRIPLET STREQUAL "x64-mingw-static")
    message(WARNING "Detected Linux compiler but cached VCPKG_TARGET_TRIPLET=x64-mingw-static. Auto-correcting to x64-linux.")
    set(VCPKG_TARGET_TRIPLET x64-linux CACHE STRING "Vcpkg target triplet" FORCE)
endif()
if(NOT DEFINED VCPKG_HOST_TRIPLET)
    if(CMAKE_HOST_WIN32)
        set(VCPKG_HOST_TRIPLET x64-windows)
    else()
        set(VCPKG_HOST_TRIPLET x64-linux)
    endif()
endif()
message("VCPKG_TARGET_TRIPLET: ${VCPKG_TARGET_TRIPLET}")
message("VCPKG_HOST_TRIPLET: ${VCPKG_HOST_TRIPLET}")
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/tools/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
include("${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

message("LD_LIBRARY_PATH:" ${LD_LIBRARY_PATH})

find_package(spdlog REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)
find_program(PNPM_EXECUTABLE pnpm)
if(NOT PNPM_EXECUTABLE)
    message(FATAL_ERROR "pnpm not found!")
endif()
# Only append .CMD when running on Windows host (not when cross-compiling to Windows from Linux)
if(CMAKE_HOST_WIN32)
    set(PNPM_EXECUTABLE "${PNPM_EXECUTABLE}.CMD")
endif()

message("CMAKE_CXX_FLAGS:${CMAKE_CXX_FLAGS}")
message("获取 cmake-js 配置")
message(${PROJECT_SOURCE_DIR})
execute_process(COMMAND
    ${PNPM_EXECUTABLE} --silent cmake-js --version
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    OUTPUT_VARIABLE CMAKE_JS_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(COMMAND
    ${PNPM_EXECUTABLE} cmake-js print-cmakejs-src --arch ${TARGET_ARCH} --silent -l error
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    OUTPUT_VARIABLE CMAKE_JS_SRC
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
message("获取 CMAKE_JS_SRC 配置: ${CMAKE_JS_SRC}")

message("PNPM_EXECUTABLE: " ${PNPM_EXECUTABLE})
execute_process(COMMAND
    ${PNPM_EXECUTABLE} cmake-js print-cmakejs-include --silent -l error
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    OUTPUT_VARIABLE CMAKE_JS_INC
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
message("CMAKE_JS_INC:" ${CMAKE_JS_INC})
foreach(item IN LISTS CMAKE_JS_INC)
    message("include: ${item}")
    include_directories(${item})
endforeach()

message("获取 node-addon-api 配置")
execute_process(COMMAND
    node -p "require('node-addon-api').include"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    OUTPUT_VARIABLE NODE_ADDON_API_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
message("node-addon-api 路径:${NODE_ADDON_API_DIR}")

message("CMAKE_JS_INC: " ${CMAKE_JS_INC})
message("CMAKE_JS_SRC: " ${CMAKE_JS_SRC})
message("CMAKE_JS_LIB: " ${CMAKE_JS_LIB})

include_directories(${CMAKE_JS_SRC})
include_directories(${NODE_ADDON_API_DIR})


add_subdirectory(src)
################test##################
