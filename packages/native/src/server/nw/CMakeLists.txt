set(NW_LIB_NAME nw.dll)
add_library(${NW_LIB_NAME} SHARED ./nw.cpp)

execute_process(COMMAND
    ${CMAKE_AR} /def:nw.original.def /out:${CMAKE_BINARY_DIR}/nw.original.lib /machine:x64 ${CMAKE_STATIC_LINKER_FLAGS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND_ERROR_IS_FATAL ANY
)
set(nw_DEF ${CMAKE_CURRENT_SOURCE_DIR}/nw.def)
if(EXISTS ${nw_DEF})
    if(MSVC)
        set_target_properties(${NW_LIB_NAME} PROPERTIES LINK_FLAGS "/DEF:${nw_DEF}")
    else()
        target_link_options(${NW_LIB_NAME} PRIVATE "-Wl,--input-def=${nw_DEF}")
    endif()
else()
    message(FATAL_ERROR "nw.def not found")
endif()
target_link_libraries(${NW_LIB_NAME} PRIVATE ${CMAKE_BINARY_DIR}/nw.original.lib)
set_target_properties(${NW_LIB_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_SOURCE_DIR}/packages/nodejs)
set_target_properties(${NW_LIB_NAME} PROPERTIES PREFIX "" SUFFIX ".dll")
