set(CLIENT_NAME skyline)
set(CLIENT_DIR_LIST
    main.cc
    client_action.cc
    client_action.hh
    client_socket.cc
    skyline_debug_info.hh
    skyline_debug_info.cc
    skyline_shell.hh
    skyline_shell.cc
    skyline_global.hh
    skyline_global.cc
    crash_handler.cc
    base_client.cc
    base_client.hh
    skyline_global/page_context.hh
    skyline_global/page_context.cc
    ../common/convert.cc
    ../common/logger.cc
    skyline_global/runtime.cc
    skyline_global/mutable_value.cc
    skyline_global/gesture_handler_module.cc
    skyline_global/worklet_module.cc
    skyline_global/async_style_sheets.cc
    skyline_global/shadow_node/node.hh
    skyline_global/shadow_node/node.cc
    skyline_global/shadow_node/view.cc
    skyline_global/shadow_node/input.cc
    skyline_global/shadow_node/scroll_view.cc
    skyline_global/shadow_node/list_view.cc
    skyline_global/shadow_node/sticky_section.cc
    skyline_global/shadow_node/sticky_header.cc
    skyline_global/shadow_node/text.cc
    skyline_global/shadow_node/image.cc
    skyline_global/shadow_node/swiper.cc
    skyline_global/shadow_node/swiper_item.cc
    skyline_global/shadow_node/grid_view.cc
    skyline_global/shadow_node/hero.cc
    skyline_global/fragment_binding.hh
    skyline_global/fragment_binding.cc
)

# Set the HEADER_DEPENDENCY_CHECK property to force rebuilding when headers change
set_source_files_properties(
    skyline_global/shadow_node/node.cc
    PROPERTIES
    OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/skyline_global/shadow_node/node.hh;${CMAKE_CURRENT_SOURCE_DIR}/base_client.hh"
)

add_library(${CLIENT_NAME}
    SHARED
    ${CLIENT_DIR_LIST}
    ${CMAKE_JS_SRC}
    )
target_include_directories(${CLIENT_NAME} PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/skyline_global/shadow_node>
)

# Enable unity builds for better header dependency tracking
set_target_properties(${CLIENT_NAME} PROPERTIES UNITY_BUILD OFF)

# Enable header dependency checking
set(CMAKE_DEPENDS_IN_PROJECT_ONLY OFF)

set(IS_RUNNING_NODE false)
if (WIN32)
    if(IS_RUNNING_NODE)
        execute_process(COMMAND
            ${CMAKE_AR} /def:node_api.def /out:node_api.lib /machine:x64 ${CMAKE_STATIC_LINKER_FLAGS}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/node_modules/node-api-headers/def
            COMMAND_ERROR_IS_FATAL ANY
        )
        target_link_libraries(${CLIENT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/node_modules/node-api-headers/def/node_api.lib)
    else()
        message("整合 node64.lib")
        target_link_libraries(${CLIENT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/thirds/nwjs/node64.lib)
    endif()
    # target_link_libraries(${CLIENT_NAME} PRIVATE bcrypt)

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