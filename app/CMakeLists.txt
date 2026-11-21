set(SRC main.cpp)

if(WIN32)
    # UTF-8 as the process code page and the application icon (taskbar, explorer)
    list(APPEND SRC hal.manifest hal.rc)
endif()

add_executable(hal ${SRC})
set_target_properties(hal PROPERTIES OUTPUT_NAME "hal")
set_target_properties(hal PROPERTIES ENABLE_EXPORTS 1)
set_target_properties(hal PROPERTIES INSTALL_RPATH "${CMAKE_INSTALL_RPATH}")
target_compile_options(hal
                       PUBLIC ${COMPILE_OPTIONS_PUBLIC}
                       PRIVATE ${COMPILE_OPTIONS_PRIVATE}
                       INTERFACE ${COMPILE_OPTIONS_INTERFACE})
target_compile_definitions(hal PUBLIC SPDLOG_TRACE_ON)
target_link_libraries(hal hal::core)

if(WIN32)
    # hal-gui.exe is the same program as a windowed application (no console window) that always opens the GUI
    add_executable(hal-gui WIN32 ${SRC})
    set_target_properties(hal-gui PROPERTIES OUTPUT_NAME "hal-gui" ENABLE_EXPORTS 1)
    target_compile_options(hal-gui
                           PUBLIC ${COMPILE_OPTIONS_PUBLIC}
                           PRIVATE ${COMPILE_OPTIONS_PRIVATE}
                           INTERFACE ${COMPILE_OPTIONS_INTERFACE})
    target_compile_definitions(hal-gui PUBLIC SPDLOG_TRACE_ON PRIVATE HAL_GUI_LAUNCHER)
    # keep main() as the entry point although the subsystem is Windows
    target_link_options(hal-gui PRIVATE /ENTRY:mainCRTStartup)
    target_link_libraries(hal-gui hal::core dbghelp)
endif()
if(WIN32)
    target_link_libraries(hal dbghelp)
endif()
install(TARGETS hal RUNTIME DESTINATION ${BINARY_INSTALL_DIRECTORY} PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE INCLUDES DESTINATION ${INCLUDE_INSTALL_DIRECTORY})
if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_sanitizers(hal)
endif()
