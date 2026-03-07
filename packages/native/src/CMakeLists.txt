
if (SKYLINE_TARGET_WINDOWS)
    add_subdirectory(server)
else()
    add_subdirectory(client)
endif()