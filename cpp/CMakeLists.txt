cmake_minimum_required(VERSION 3.20)
project(poker_core CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 优化选项
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        add_compile_options(/O2 /arch:AVX2 /DNDEBUG /EHsc /MP /W3)
    else()
        add_compile_options(-O3 -march=native -DNDEBUG)
    endif()
endif()

# 头文件路径
include_directories(${CMAKE_SOURCE_DIR}/include)

# 查找 Python 3.10
if(DEFINED ENV{CONDA_PREFIX})
    set(Python_ROOT_DIR $ENV{CONDA_PREFIX})
    message(STATUS "Using Python from CONDA_PREFIX: $ENV{CONDA_PREFIX}")
endif()
find_package(Python 3.10 EXACT REQUIRED COMPONENTS Interpreter Development)

# 源文件
set(SOURCES
    src/card.cpp
    src/hand_evaluator.cpp
    src/game_state.cpp
    src/cfr_engine.cpp
    src/environment.cpp
    src/win_rate.cpp
)

# 静态库
add_library(poker_core_static STATIC ${SOURCES})

# ========== pybind11 Python 绑定 ==========
# 如果 CMake 找不到，手动指定 conda 里的路径
find_package(pybind11 QUIET)
if(NOT pybind11_FOUND)
    if(DEFINED ENV{CONDA_PREFIX})
        set(pybind11_DIR "$ENV{CONDA_PREFIX}/Lib/site-packages/pybind11/share/cmake/pybind11")
        find_package(pybind11 REQUIRED)
    else()
        message(FATAL_ERROR "pybind11 not found. Run: pip install pybind11")
    endif()
endif()

pybind11_add_module(_core python_bindings/bindings.cpp)
target_link_libraries(_core PRIVATE poker_core_static)

# 让 .pyd 直接生成到 ../poker_core/，方便 Python import
set_target_properties(_core PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../poker_core
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../poker_core
)

# ========== C++ 单元测试（每个测试独立可执行文件） ==========
enable_testing()

add_executable(test_hand_eval tests/test_hand_eval.cpp)
target_link_libraries(test_hand_eval PRIVATE poker_core_static)
add_test(NAME test_hand_eval COMMAND test_hand_eval)

add_executable(test_cfr tests/test_cfr.cpp)
target_link_libraries(test_cfr PRIVATE poker_core_static)
add_test(NAME test_cfr COMMAND test_cfr)