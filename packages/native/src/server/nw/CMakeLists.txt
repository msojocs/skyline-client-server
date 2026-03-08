set(NW_LIB_NAME nw)
add_library(${NW_LIB_NAME} SHARED ./nw.cpp)

if(SKYLINE_USING_MINGW)
    # MinGW cross-compilation: use dlltool to generate import library
    find_program(DLLTOOL_EXECUTABLE NAMES x86_64-w64-mingw32-dlltool dlltool)
    if(NOT DLLTOOL_EXECUTABLE)
        message(FATAL_ERROR "dlltool not found, required for MinGW cross-compilation")
    endif()
    execute_process(COMMAND
        ${DLLTOOL_EXECUTABLE} -d ${CMAKE_CURRENT_SOURCE_DIR}/nw.original.def
        -l ${CMAKE_BINARY_DIR}/libnw.original.a -m i386:x86-64
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND_ERROR_IS_FATAL ANY
    )
    set(NW_ORIGINAL_LIB ${CMAKE_BINARY_DIR}/libnw.original.a)
elseif(MSVC)
    execute_process(COMMAND
        ${CMAKE_AR} /def:nw.original.def /out:${CMAKE_BINARY_DIR}/nw.original.lib /machine:x64 ${CMAKE_STATIC_LINKER_FLAGS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND_ERROR_IS_FATAL ANY
    )
    set(NW_ORIGINAL_LIB ${CMAKE_BINARY_DIR}/nw.original.lib)
else()
    message(FATAL_ERROR "Unsupported Windows compiler for nw wrapper target")
endif()
set(nw_DEF ${CMAKE_CURRENT_SOURCE_DIR}/nw.def)
if(EXISTS ${nw_DEF})
    if(MSVC)
        set_target_properties(${NW_LIB_NAME} PROPERTIES LINK_FLAGS "/DEF:${nw_DEF}")
    elseif(SKYLINE_USING_MINGW)
        target_link_libraries(${NW_LIB_NAME} PRIVATE "${nw_DEF}")
    else()
        message(FATAL_ERROR "Unsupported compiler for nw.def handling")
    endif()
else()
    message(FATAL_ERROR "nw.def not found")
endif()
target_link_libraries(${NW_LIB_NAME} PRIVATE ${NW_ORIGINAL_LIB})
set_target_properties(${NW_LIB_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_SOURCE_DIR}/../nodejs)
set_target_properties(${NW_LIB_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY  ${CMAKE_SOURCE_DIR}/../nodejs)
set_target_properties(${NW_LIB_NAME} PROPERTIES PREFIX "" SUFFIX ".dll")
