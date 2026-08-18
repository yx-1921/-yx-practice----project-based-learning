# 3rdparty 依赖库根目录
set(THIRDPARTY_DIR "${CMAKE_SOURCE_DIR}/tinySDLGame/3rdparty")

# 让 find_package 能在 3rdparty 中找到各库的 Config 文件
list(APPEND CMAKE_PREFIX_PATH
    "${THIRDPARTY_DIR}/SDL3-3.4.14"
    "${THIRDPARTY_DIR}/SDL3_image-3.4.4"
    "${THIRDPARTY_DIR}/SDL3_mixer-3.2.4"
    "${THIRDPARTY_DIR}/SDL3_ttf-3.1.0")

find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
find_package(SDL3_mixer CONFIG REQUIRED)
find_package(SDL3_ttf CONFIG REQUIRED)

# tmxlite 以源码形式构建（生成 tmxlite 目标，头文件目录自动传播）
set(TMXLITE_STATIC_LIB TRUE)
add_subdirectory("${THIRDPARTY_DIR}/tmxlite-1.4.5/tmxlite")

# 将源代码添加到此项目的可执行文件。
add_executable (tinySDLGame "main.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET tinySDLGame PROPERTY CXX_STANDARD 20)
endif()

target_link_libraries(tinySDLGame PRIVATE
    SDL3::SDL3
    SDL3_image::SDL3_image
    SDL3_mixer::SDL3_mixer
    SDL3_ttf::SDL3_ttf
    tmxlite)

# 把 SDL3.dll 复制到可执行文件目录，否则运行时找不到 DLL
if(WIN32)
    add_custom_command(TARGET tinySDLGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:SDL3::SDL3-shared>"
            "$<TARGET_FILE_DIR:tinySDLGame>"
        VERBATIM
    )
endif()
