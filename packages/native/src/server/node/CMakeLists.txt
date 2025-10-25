set(NODE_LIB_NAME node.dll)
add_library(${NODE_LIB_NAME} SHARED ./node.cpp)
execute_process(COMMAND
    ${CMAKE_AR} /def:node.original.def /out:${CMAKE_BINARY_DIR}/node.original.lib /machine:x64 ${CMAKE_STATIC_LINKER_FLAGS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND_ERROR_IS_FATAL ANY
)
set(node_DEF ${CMAKE_CURRENT_SOURCE_DIR}/node.def)
if(EXISTS ${node_DEF})
    if(MSVC)
        set_target_properties(${NODE_LIB_NAME} PROPERTIES LINK_FLAGS "/DEF:${node_DEF}")
    else()
        target_link_options(${NODE_LIB_NAME} PRIVATE "-Wl,--input-def=${node_DEF}")
    endif()
else()
    message(FATAL_ERROR "node.def not found")
endif()
target_link_libraries(${NODE_LIB_NAME} PRIVATE ${CMAKE_BINARY_DIR}/node.original.lib)
set_target_properties(${NODE_LIB_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/packages/nodejs)
set_target_properties(${NODE_LIB_NAME} PROPERTIES PREFIX "" SUFFIX ".dll")
