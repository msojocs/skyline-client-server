cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0042 NEW)
# cmake_policy(SET CMP0167 NEW)

project("module_example")

set(VCPKG_ROOT $ENV{VCPKG_ROOT})
message("VCPKG_ROOT: " ${VCPKG_ROOT})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CXX_STANDARD 17)
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
find_package(nlohmann_json CONFIG REQUIRED)
find_package(ixwebsocket CONFIG REQUIRED)
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

set(CLIENT_NAME skylineClient)
set(CLIENT_DIR_LIST
    src/main.cc
    src/websocket.cc
    src/websocket.hh
    src/skyline_debug_info.hh
    src/skyline_debug_info.cc
    src/skyline_shell.hh
    src/skyline_shell.cc
    src/skyline_global.hh
    src/skyline_global.cc
    src/base_client.cc
    src/include/base_client.hh
    src/include/page_context.hh
    src/skyline_global/page_context.cc
    src/skyline_global/convert.cc
    src/skyline_global/runtime.cc
)

add_library(${CLIENT_NAME}
    SHARED
    ${CLIENT_DIR_LIST}
    ${CMAKE_JS_SRC}
    )

set(IS_RUNNING_NODE true)
if (WIN32)
    if(IS_RUNNING_NODE)
        message("生成 node_api.lib")
        execute_process(COMMAND
            ${CMAKE_AR} /def:node_api.def /out:node_api.lib /machine:x86 ${CMAKE_STATIC_LINKER_FLAGS}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/node_modules/node-api-headers/def
            COMMAND_ERROR_IS_FATAL ANY
        )
        target_link_libraries(${CLIENT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/node_modules/node-api-headers/def/node_api.lib)
    else()
        message("整合 node64.lib")
        target_link_libraries(${CLIENT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/thirds/nwjs/node64.lib)
    endif()
    target_link_libraries(${CLIENT_NAME} PRIVATE bcrypt)

else()
    # Linux specific libraries
    target_link_libraries(${CLIENT_NAME} PRIVATE rt pthread)
endif()

target_link_libraries(${CLIENT_NAME} PRIVATE spdlog::spdlog)
target_link_libraries(${CLIENT_NAME} PRIVATE ixwebsocket::ixwebsocket)
target_link_libraries(${CLIENT_NAME} PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(${CLIENT_NAME} PRIVATE ${CMAKE_JS_LIB})

set_target_properties(${CLIENT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

add_subdirectory(src/server)
################test##################
