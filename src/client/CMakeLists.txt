
set(CLIENT_NAME skylineClient)
set(CLIENT_DIR_LIST
    client.cc
    websocket.cc
    websocket.hh
    skyline_debug_info.hh
    skyline_debug_info.cc
    skyline_shell.hh
    skyline_shell.cc
    skyline_global.hh
    skyline_global.cc
    base_client.cc
    include/base_client.hh
    include/page_context.hh
    skyline_global/page_context.cc
    ../common/convert.cc
    ../common/logger.cc
    skyline_global/runtime.cc
    skyline_global/async_style_sheets.cc
    skyline_global/shadow_node/node.cc
    skyline_global/shadow_node/view.cc
    skyline_global/shadow_node/text.cc
    include/fragment_binding.hh
    skyline_global/fragment_binding.cc
)

add_library(${CLIENT_NAME}
    SHARED
    ${CLIENT_DIR_LIST}
    ${CMAKE_JS_SRC}
    )
set(IS_RUNNING_NODE false)
if (WIN32)
    if(IS_RUNNING_NODE)
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

# 添加_SKYLINE_CLIENT_标记
target_compile_definitions(${CLIENT_NAME} PRIVATE _SKYLINE_CLIENT_)
target_link_libraries(${CLIENT_NAME} PRIVATE spdlog::spdlog)
target_link_libraries(${CLIENT_NAME} PRIVATE ixwebsocket::ixwebsocket)
target_link_libraries(${CLIENT_NAME} PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(${CLIENT_NAME} PRIVATE ${CMAKE_JS_LIB})
# 设定输出目录为build
set_target_properties(${CLIENT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build)
set_target_properties(${CLIENT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")