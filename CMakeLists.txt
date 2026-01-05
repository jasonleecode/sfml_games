# SnakeGame/CMakeLists.txt

cmake_minimum_required(VERSION 3.10)
project(SnakeGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 查找 SFML 模块 (需要 system, window 和 graphics 组件)
find_package(SFML REQUIRED System Window Graphics)

# ----------------------------------------------------
# 创建可执行文件
# ----------------------------------------------------
add_executable(SnakeGame main.cpp)

# ----------------------------------------------------
# 链接 SFML 库
# ----------------------------------------------------
target_link_libraries(SnakeGame PRIVATE 
    SFML::System 
    SFML::Window 
    SFML::Graphics
)