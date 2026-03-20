set(CLIENT_NAME skyline)
set(CLIENT_DIR_LIST
    main.cc
    client_action.cc
    client_action.hh
    client_socket.cc
    controller.cc
    crash_handler.cc
    base_client.cc
    base_client.hh
    ../common/convert.cc
    ../common/logger.cc
    html/node.cc
    html/controller.cc
    html/css_style_declaration.cc
    html/event.cc
    html/webview_element.cc
    html/web_request_event.cc
    html/request_rule.cc
    html/request_message_event.cc
)

# Set the HEADER_DEPENDENCY_CHECK property to force rebuilding when headers change
set_source_files_properties(
    html/node.cc
    PROPERTIES
    OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/html/node.hh;${CMAKE_CURRENT_SOURCE_DIR}/html/webview_element.hh;${CMAKE_CURRENT_SOURCE_DIR}/base_client.hh"
)

add_library(${CLIENT_NAME}
    SHARED
    ${CLIENT_DIR_LIST}
    ${CMAKE_JS_SRC}
    )
target_include_directories(${CLIENT_NAME} PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/html>
)

# Enable unity builds for better header dependency tracking
set_target_properties(${CLIENT_NAME} PROPERTIES UNITY_BUILD OFF)

# Enable header dependency checking
set(CMAKE_DEPENDS_IN_PROJECT_ONLY OFF)

set(IS_RUNNING_NODE false)
if (SKYLINE_TARGET_WINDOWS)
    if(IS_RUNNING_NODE)
        if(SKYLINE_USING_MINGW)
            # MinGW cross-compilation: generate import library from .def
            find_program(DLLTOOL_EXECUTABLE NAMES x86_64-w64-mingw32-dlltool dlltool)
            if(DLLTOOL_EXECUTABLE)
                execute_process(COMMAND
                    ${DLLTOOL_EXECUTABLE} -d ${PROJECT_SOURCE_DIR}/node_modules/node-api-headers/def/node_api.def
                    -l ${CMAKE_BINARY_DIR}/libnode_api_client.a -m i386:x86-64
                    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/node_modules/node-api-headers/def
                    COMMAND_ERROR_IS_FATAL ANY
                )
                target_link_libraries(${CLIENT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/libnode_api_client.a)
            else()
                message(FATAL_ERROR "dlltool not found, required for MinGW cross-compilation")
            endif()
        elseif(MSVC)
            execute_process(COMMAND
                ${CMAKE_AR} /def:node_api.def /out:node_api.lib /machine:x64 ${CMAKE_STATIC_LINKER_FLAGS}
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/node_modules/node-api-headers/def
                COMMAND_ERROR_IS_FATAL ANY
            )
            target_link_libraries(${CLIENT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/node_modules/node-api-headers/def/node_api.lib)
        else()
            message(FATAL_ERROR "Unsupported Windows compiler for client target")
        endif()
    else()
        message("整合 node64.lib")
        target_link_libraries(${CLIENT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/thirds/nwjs/node64.lib)
    endif()
    # target_link_libraries(${CLIENT_NAME} PRIVATE bcrypt)
    target_link_libraries(${CLIENT_NAME} PRIVATE dbghelp)
    if(SKYLINE_USING_MINGW)
        target_link_libraries(${CLIENT_NAME} PRIVATE ws2_32 wsock32)
    endif()

else()
    # Linux specific libraries
    target_link_libraries(${CLIENT_NAME} PRIVATE rt pthread)
endif()

# 添加_SKYLINE_CLIENT_标记
target_compile_definitions(${CLIENT_NAME} PRIVATE _SKYLINE_CLIENT_)
target_link_libraries(${CLIENT_NAME} PRIVATE spdlog::spdlog)
find_package(Boost REQUIRED COMPONENTS asio thread)
target_link_libraries(${CLIENT_NAME} PRIVATE Boost::asio Boost::thread)
target_link_libraries(${CLIENT_NAME} PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(${CLIENT_NAME} PRIVATE ${CMAKE_JS_LIB})
# 设定输出目录为build
message("Output env: $ENV{SKYLINE_DEV_PATH}")
if ("$ENV{SKYLINE_DEV_PATH}" STREQUAL "")
    message("Set output path: ${PROJECT_SOURCE_DIR}/build")
    set_target_properties(${CLIENT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build)
    set_target_properties(${CLIENT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build)
else()
    message("Set output path: $ENV{SKYLINE_DEV_PATH}")
    set_target_properties(${CLIENT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY $ENV{SKYLINE_DEV_PATH})
    set_target_properties(${CLIENT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY $ENV{SKYLINE_DEV_PATH})
endif()
set_target_properties(${CLIENT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
