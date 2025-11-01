
set(SERVER_NAME skylineServer)
set(SERVER_DIR_LIST    server.cc
    socket_server.cc
    socket_server.hh
    ../common/convert.cc
    ../common/logger.cc
)

add_library(${SERVER_NAME}
    SHARED
    ${SERVER_DIR_LIST}
    ${CMAKE_JS_SRC}
    )

set(IS_RUNNING_NODE false)
if (WIN32)
    if(IS_RUNNING_NODE)
        target_link_libraries(${SERVER_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/node_modules/node-api-headers/def/node_api.lib)
    else()
        message("整合 node64.lib")
        target_link_libraries(${SERVER_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/thirds/nwjs/node64.lib)
    endif()
    target_link_libraries(${SERVER_NAME} PRIVATE bcrypt)

else()
    # Linux specific libraries
    target_link_libraries(${SERVER_NAME} PRIVATE rt pthread)
endif()

target_link_libraries(${SERVER_NAME} PRIVATE spdlog::spdlog)
find_package(Boost REQUIRED COMPONENTS asio thread)
target_link_libraries(${SERVER_NAME} PRIVATE Boost::asio Boost::thread)
target_link_libraries(${SERVER_NAME} PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(${SERVER_NAME} PRIVATE ${CMAKE_JS_LIB})
# 设定输出目录为build
set_target_properties(${SERVER_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../nodejs/node_modules/skyline-server)
set_target_properties(${SERVER_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../nodejs/node_modules/skyline-server)
set_target_properties(${SERVER_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

add_subdirectory(node)
add_subdirectory(nw)