import os
import sys
import platform
from setuptools import setup, find_packages, Extension
from pybind11.setup_helpers import Pybind11Extension, build_ext
import pybind11

# 检测平台和编译器
is_windows = platform.system() == "Windows"
is_msvc = is_windows and ("MSC" in sys.version)

# Python版本检查
if sys.version_info < (3, 10) or sys.version_info >= (3, 11):
    raise RuntimeError(
        "This project requires Python 3.10.x exactly. "
        "Current version: %s" % sys.version
    )

# C++ 源文件
sources = [
    "cpp/python_bindings/bindings.cpp",
    "cpp/src/card.cpp",
    "cpp/src/hand_evaluator.cpp",
    "cpp/src/game_state.cpp",
    "cpp/src/cfr_engine.cpp",
    "cpp/src/environment.cpp",
    "cpp/src/win_rate.cpp",
]

# 编译参数
if is_msvc:
    extra_compile = ["/O2", "/arch:AVX2", "/DNDEBUG", "/EHsc", "/MP", "/W3", "/utf-8"]
    extra_link = []
else:
    extra_compile = ["-O3", "-march=native", "-DNDEBUG", "-std=c++17"]
    extra_link = []

ext_modules = [
    Pybind11Extension(
        "poker_core._core",
        sources=sources,
        include_dirs=[
            "cpp/include",
            pybind11.get_include(),
        ],
        cxx_std=17,
        extra_compile_args=extra_compile,
        extra_link_args=extra_link,
    ),
]

setup(
    name="poker_core",
    version="0.1.0",
    packages=find_packages(),
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
    python_requires=">=3.10,<3.11",
)
