cmake_minimum_required(VERSION 3.20)

win32_library(TARGET_NAME alx-home_windows
    FILES 
        src/Env.cpp
        src/SystemTray.cpp
        src/Shortcuts.cpp
        src/Lock.cpp
        src/Process.cpp
        src/JumpList.cpp

    COMPILE_OPTIONS
        -D_AFXDLL
)

target_include_directories(alx-home_windows PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(alx-home_windows PUBLIC alx-home::cpp_utils)
add_library(alx-home::windows ALIAS alx-home_windows)
