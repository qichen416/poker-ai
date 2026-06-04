# 最低 CMake 版本要求
cmake_minimum_required(VERSION 3.20)
# 项目名称与语言
project(poker_core CXX)

# 使用 C++17 标准，且必须满足
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------- 编译优化选项 ----------
# 如果未指定构建类型，默认设为 Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        # MSVC 编译器优化选项：
        # /O2 最大速度优化，/arch:AVX2 启用 AVX2 指令集，
        # /DNDEBUG 禁用断言，/EHsc 启用 C++ 异常处理，
        # /MP 多核编译，/W3 警告级别 3
        add_compile_options(/O2 /arch:AVX2 /DNDEBUG /EHsc /MP /W3)
    else()
        # GCC/Clang：-O3 最高优化，-march=native 生成本机 CPU 指令，
        # -DNDEBUG 禁用断言
        add_compile_options(-O3 -march=native -DNDEBUG)
    endif()
endif()

# ---------- 头文件路径 ----------
# 包含项目根目录下的 include 文件夹
include_directories(${CMAKE_SOURCE_DIR}/include)

# ---------- 查找 Python 3.10 ----------
# 若在 Conda 环境中，优先使用该环境的 Python
if(DEFINED ENV{CONDA_PREFIX})
    set(Python_ROOT_DIR $ENV{CONDA_PREFIX})
    message(STATUS "Using Python from CONDA_PREFIX: $ENV{CONDA_PREFIX}")
endif()

# 如果仍未指定，尝试自动探测常见位置的 poker conda 环境
if(NOT DEFINED Python_ROOT_DIR AND NOT DEFINED Python_EXECUTABLE)
    set(_POSSIBLE_CONDA_ENVS
        "$ENV{USERPROFILE}/anaconda3/envs/poker"
        "$ENV{USERPROFILE}/miniconda3/envs/poker"
        "$ENV{LOCALAPPDATA}/Continuum/anaconda3/envs/poker"
        "C:/ProgramData/anaconda3/envs/poker"
        "E:/anaconda3/envs/poker"
    )
    foreach(_ENV_PATH ${_POSSIBLE_CONDA_ENVS})
        if(EXISTS "${_ENV_PATH}/python.exe")
            set(Python_ROOT_DIR "${_ENV_PATH}")
            message(STATUS "Auto-detected Python 3.10 from conda env: ${_ENV_PATH}")
            break()
        endif()
    endforeach()
endif()

find_package(Python 3.10 EXACT REQUIRED COMPONENTS Interpreter Development)

# ---------- 源文件列表 ----------
set(SOURCES
    src/card.cpp
    src/hand_evaluator.cpp
    src/game_state.cpp
    src/cfr_engine.cpp
    src/environment.cpp
    src/win_rate.cpp
)

# ---------- 构建静态库 ----------
add_library(poker_core_static STATIC ${SOURCES})

# ========== pybind11 Python 绑定 ==========
# 先尝试自动查找
find_package(pybind11 QUIET)

# 如果没找到，且存在 CONDA_PREFIX，则手动设置查找路径
if(NOT pybind11_FOUND AND DEFINED ENV{CONDA_PREFIX})
    set(pybind11_DIR "$ENV{CONDA_PREFIX}/Lib/site-packages/pybind11/share/cmake/pybind11")
    find_package(pybind11 REQUIRED)
endif()

# 若仍未找到，尝试常见路径 + Python 查询（更健壮的方法）
if(NOT pybind11_FOUND)
    set(_PYBIND_CANDIDATES
        "$ENV{CONDA_PREFIX}/Lib/site-packages/pybind11/share/cmake/pybind11"
        "$ENV{CONDA_PREFIX}/lib/site-packages/pybind11/share/cmake/pybind11"
    )
    # 把自动探测到的 Python 环境也加进来
    if(DEFINED Python_ROOT_DIR)
        list(APPEND _PYBIND_CANDIDATES
            "${Python_ROOT_DIR}/Lib/site-packages/pybind11/share/cmake/pybind11"
            "${Python_ROOT_DIR}/lib/site-packages/pybind11/share/cmake/pybind11"
        )
    endif()
    foreach(_PYBIND_DIR ${_PYBIND_CANDIDATES})
        if(EXISTS "${_PYBIND_DIR}/pybind11Config.cmake")
            set(pybind11_DIR "${_PYBIND_DIR}")
            find_package(pybind11 REQUIRED)
            break()
        endif()
    endforeach()
endif()

# 最后 fallback：用 Python 解释器直接查询 pybind11 的 cmake 目录
if(NOT pybind11_FOUND)
    execute_process(
        COMMAND "${Python_EXECUTABLE}" -c "import pybind11; print(pybind11.get_cmake_dir())"
        OUTPUT_VARIABLE pybind11_CMAKE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _PYBIND_RESULT
    )
    if(_PYBIND_RESULT EQUAL 0 AND EXISTS "${pybind11_CMAKE_DIR}/pybind11Config.cmake")
        set(pybind11_DIR ${pybind11_CMAKE_DIR})
        find_package(pybind11 REQUIRED)
    else()
        message(FATAL_ERROR "Could not find pybind11. Please install it in your Python 3.10 environment (pip install pybind11).")
    endif()
endif()

# 用 pybind11 创建一个 Python 模块 _core（最终生成 _core.pyd 或 _core.so）
pybind11_add_module(_core python_bindings/bindings.cpp)
# 链接到前面生成的静态库
target_link_libraries(_core PRIVATE poker_core_static)

# 将生成的 .pyd/.so 输出到 ../poker_core 目录，方便 Python 直接 import
set_target_properties(_core PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../poker_core
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../poker_core
)

# ========== C++ 单元测试 ==========
enable_testing()

# 测试可执行文件：手牌评估测试
add_executable(test_hand_eval tests/test_hand_eval.cpp)
target_link_libraries(test_hand_eval PRIVATE poker_core_static)
add_test(NAME test_hand_eval COMMAND test_hand_eval)

# 测试可执行文件：CFR 引擎测试
add_executable(test_cfr tests/test_cfr.cpp)
target_link_libraries(test_cfr PRIVATE poker_core_static)
add_test(NAME test_cfr COMMAND test_cfr)