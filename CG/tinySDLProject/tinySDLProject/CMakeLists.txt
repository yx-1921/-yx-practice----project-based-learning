# tinyProject 的 CMake 项目：编译 main.cpp 并链接 SDL3

add_executable (tinyProject "main.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET tinyProject PROPERTY CXX_STANDARD 20)
endif()

# ---- SDL3 ----
set(SDL3_DIR "${CMAKE_SOURCE_DIR}/SDL3-3.4.14/cmake")

# 官方预编译包的 SDL3Config.cmake 靠 SDL_CPU_* 选择 lib/x64 或 lib/x86，但不会自动检测，这里按指针宽度补上
if(NOT SDL_CPU_X86 AND NOT SDL_CPU_X64 AND NOT SDL_CPU_ARM64)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(SDL_CPU_X64 ON CACHE BOOL "" FORCE)
    else()
        set(SDL_CPU_X86 ON CACHE BOOL "" FORCE)
    endif()
endif()

find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(tinyProject PRIVATE SDL3::SDL3)

# 把 SDL3.dll 复制到可执行文件目录，否则运行时找不到 DLL
if(WIN32)
    add_custom_command(TARGET tinyProject POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:SDL3::SDL3-shared>"
            "$<TARGET_FILE_DIR:tinyProject>"
        VERBATIM
    )
endif()
