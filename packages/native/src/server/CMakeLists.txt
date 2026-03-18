
set(SERVER_NAME server)
set(SERVER_DIR_LIST
    main.cc
    server_action.cc
    server_action.hh
    server_socket.cc
    ../common/convert.cc
    ../common/logger.cc
)

add_library(${SERVER_NAME}
    SHARED
    ${SERVER_DIR_LIST}
    ${CMAKE_JS_SRC}
    )

set(IS_RUNNING_NODE false)
if (SKYLINE_TARGET_WINDOWS)
    if(IS_RUNNING_NODE)
        if(SKYLINE_USING_MINGW)
            # MinGW cross-compilation: generate import library from .def
            find_program(DLLTOOL_EXECUTABLE NAMES x86_64-w64-mingw32-dlltool dlltool)
            if(DLLTOOL_EXECUTABLE)
                execute_process(COMMAND
                    ${DLLTOOL_EXECUTABLE} -d ${PROJECT_SOURCE_DIR}/node_modules/node-api-headers/def/node_api.def
                    -l ${CMAKE_BINARY_DIR}/libnode_api.a -m i386:x86-64
                    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/node_modules/node-api-headers/def
                    COMMAND_ERROR_IS_FATAL ANY
                )
                target_link_libraries(${SERVER_NAME} PRIVATE ${CMAKE_BINARY_DIR}/libnode_api.a)
            else()
                message(FATAL_ERROR "dlltool not found, required for MinGW cross-compilation")
            endif()
        else()
            target_link_libraries(${SERVER_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/node_modules/node-api-headers/def/node_api.lib)
        endif()
    else()
        message("整合 node64.lib")
        if(SKYLINE_USING_MINGW)
            # MinGW cross-compilation: generate import library from node64.lib
            # MinGW linker can use MSVC .lib files directly in many cases
            target_link_libraries(${SERVER_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/thirds/nwjs/node64.lib)
        else()
            target_link_libraries(${SERVER_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/thirds/nwjs/node64.lib)
        endif()
    endif()
    target_link_libraries(${SERVER_NAME} PRIVATE bcrypt)
    if(SKYLINE_USING_MINGW)
        target_link_libraries(${SERVER_NAME} PRIVATE ws2_32 wsock32)
    endif()

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
set_target_properties(${SERVER_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../nwjs/node_modules/skyline-server)
set_target_properties(${SERVER_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../nwjs/node_modules/skyline-server)
set_target_properties(${SERVER_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
