cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0042 NEW)
cmake_policy(SET CMP0167 NEW)

project("module_skyline")

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
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
set(CMAKE_VERBOSE_MAKEFILE ON)
set(PKG_TOOL pnpm)
set(TARGET_ARCH x64)
if(MSVC)
    execute_process(COMMAND
        chcp 65001
    )
    set(VCPKG_TARGET_TRIPLET x64-windows-static)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
else()
    # Linux specific settings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_REENTRANT")
endif()
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
include("${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

message("LD_LIBRARY_PATH:" ${LD_LIBRARY_PATH})

find_package(spdlog REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(Protobuf REQUIRED)
find_package(absl REQUIRED)

# Generate protobuf files
set(PROTO_FILES src/common/messages.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

# Create a protobuf library that can be linked by both client and server
add_library(skyline_protobuf STATIC ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(skyline_protobuf 
    protobuf::libprotoc
    protobuf::libprotobuf
)
target_include_directories(skyline_protobuf PUBLIC 
    ${Protobuf_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Include protobuf headers
include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

find_program(PNPM_EXECUTABLE pnpm)
if(NOT PNPM_EXECUTABLE)
    message(FATAL_ERROR "pnpm not found!")
endif()
if(WIN32)
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
