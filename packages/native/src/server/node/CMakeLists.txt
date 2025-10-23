
add_library(node SHARED ./node.cpp)
execute_process(COMMAND
    ${CMAKE_AR} /def:node.original.def /out:${CMAKE_BINARY_DIR}/node.original.lib /machine:x64 ${CMAKE_STATIC_LINKER_FLAGS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND_ERROR_IS_FATAL ANY
)
set(node_DEF ${CMAKE_CURRENT_SOURCE_DIR}/node.def)
if(EXISTS ${node_DEF})
    if(MSVC)
        set_target_properties(node PROPERTIES LINK_FLAGS "/DEF:${node_DEF}")
    else()
        target_link_options(node PRIVATE "-Wl,--input-def=${node_DEF}")
    endif()
else()
    message(FATAL_ERROR "node.def not found")
endif()
target_link_libraries(node PRIVATE ${CMAKE_BINARY_DIR}/node.original.lib)
set_target_properties(node PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/packages/nodejs)
set_target_properties(node PROPERTIES PREFIX "" SUFFIX ".dll")
