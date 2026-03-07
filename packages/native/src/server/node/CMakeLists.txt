set(NODE_LIB_NAME node)
add_library(${NODE_LIB_NAME} SHARED ./node.cpp)
if(SKYLINE_USING_MINGW)
    # MinGW cross-compilation: use dlltool to generate import library
    find_program(DLLTOOL_EXECUTABLE NAMES x86_64-w64-mingw32-dlltool dlltool)
    if(NOT DLLTOOL_EXECUTABLE)
        message(FATAL_ERROR "dlltool not found, required for MinGW cross-compilation")
    endif()
    execute_process(COMMAND
        ${DLLTOOL_EXECUTABLE} -d ${CMAKE_CURRENT_SOURCE_DIR}/node.original.def
        -l ${CMAKE_BINARY_DIR}/libnode.original.a -m i386:x86-64
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND_ERROR_IS_FATAL ANY
    )
    set(NODE_ORIGINAL_LIB ${CMAKE_BINARY_DIR}/libnode.original.a)
elseif(MSVC)
    execute_process(COMMAND
        ${CMAKE_AR} /def:node.original.def /out:${CMAKE_BINARY_DIR}/node.original.lib /machine:x64 ${CMAKE_STATIC_LINKER_FLAGS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND_ERROR_IS_FATAL ANY
    )
    set(NODE_ORIGINAL_LIB ${CMAKE_BINARY_DIR}/node.original.lib)
else()
    message(FATAL_ERROR "Unsupported Windows compiler for node wrapper target")
endif()
set(node_DEF ${CMAKE_CURRENT_SOURCE_DIR}/node.def)
if(EXISTS ${node_DEF})
    if(MSVC)
        set_target_properties(${NODE_LIB_NAME} PROPERTIES LINK_FLAGS "/DEF:${node_DEF}")
    elseif(SKYLINE_USING_MINGW)
        # MinGW: add .def as source file for the shared library
        target_sources(${NODE_LIB_NAME} PRIVATE ${node_DEF})
    else()
        message(FATAL_ERROR "Unsupported compiler for node.def handling")
    endif()
else()
    message(FATAL_ERROR "node.def not found")
endif()
target_link_libraries(${NODE_LIB_NAME} PRIVATE ${NODE_ORIGINAL_LIB})
set_target_properties(${NODE_LIB_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../nodejs)
set_target_properties(${NODE_LIB_NAME} PROPERTIES PREFIX "" SUFFIX ".dll")
