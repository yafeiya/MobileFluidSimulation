# 指定 CMake 的最低版本要求为 3.22.1。
cmake_minimum_required(VERSION 3.22.1)

# 定义项目名称为 "mytest"。
project("mytest")

# 构建 Vulkan 应用
get_filename_component(REPO_ROOT_DIR
        ${CMAKE_SOURCE_DIR}/../../../../..  ABSOLUTE)  # 获取仓库根目录的绝对路径
set(COMMON_DIR ${REPO_ROOT_DIR}/mytest/app)  # 设置一个变量指向公共目录
message(${CMAKE_PROJECT_NAME})
message(${COMMON_DIR})

set(CMAKE_BUILD_TYPE Debug) ##
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g") ##
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall \
                     -DVK_USE_PLATFORM_ANDROID_KHR")  # 设置编译器标志，启用 C++17 标准，开启所有警告，定义 Vulkan 平台为 Android

# 搜索 game-activity 包，这个包由 game activity 依赖提供。
find_package(game-activity REQUIRED CONFIG)
set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -u \
    Java_com_google_androidgamesdk_GameActivity_initializeNativeCode")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#ARCHIVE_OUTPUT_DIRECTORY_RELEASE (${OUT_LIB_PATH}/release)
#set(OUT_LIB_PATH ${PROJECT_SOURCE_DIR}/lib)
#message(${OUT_LIB_PATH})
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/libs/${ANDROID_ABI})
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/${ANDROID_ABI})
#message(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
#message(${CMAKE_SOURCE_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${COMMON_DIR}/src/main/jniLibs/${ANDROID_ABI})

add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR=1)

# 创建一个名为 "mytest" 的共享库，这个名字必须与 Kotlin/Java 代码中
# 加载的库名或 AndroidManifest.txt 文件中的名称一致。
add_library(mytest SHARED

        extensionfuncs.cpp
        helperfuncs.cpp
        renderer.cpp
        android_main.cpp
)

# 找到 Vulkan 库
find_package(Vulkan REQUIRED)
find_library(Vulkan_LIBRARY NAMES vulkan)
target_link_libraries(mytest  ${Vulkan_LIBRARY})

## 添加头文件搜索路径
#include_directories(${COMMON_DIR}/src/main/pk/glm)
#include_directories(${COMMON_DIR}/src/main/pk/vulkan)
#include_directories(${COMMON_DIR}/src/main/cpp/3rdparty/stb)


# 配置 CMake 用于链接目标库的库。
target_link_libraries(mytest
        # game activity 库
        game-activity::game-activity_static
        Vulkan::Vulkan
        # 用于绘图和与 Android 系统交互所需的 EGL 和其他依赖库
        EGL
        GLESv3
        jnigraphics
        android
        log)

# 如果需要，添加包含目录
target_include_directories(mytest PRIVATE
        ${Vulkan_INCLUDE_DIRS}
        ${COMMON_DIR}/src/main/pk/glm
        ${COMMON_DIR}/src/main/pk/vulkan
        ${COMMON_DIR}/src/main/cpp/3rdparty/stb
)
## 添加头文件搜索路径
#include_directories(${COMMON_DIR}/src/main/pk/glm)
#include_directories(${COMMON_DIR}/src/main/pk/vulkan)
#include_directories(${COMMON_DIR}/src/main/cpp/3rdparty/stb)