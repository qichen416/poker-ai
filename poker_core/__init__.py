'''
C++核心库的Python包装 (Python 3.10 + CUDA 12.6)。
'''
import sys

if sys.version_info < (3, 10) or sys.version_info >= (3, 11):
    raise ImportError(
        "This package requires Python 3.10.x exactly. "
        "Current version: %s" % sys.version
    )

try:
    from poker_core._core import (
        Card, create_deck, parse_protocol_cards,
        HandEvaluator, HandResult,
        GameState, ActionRecord, Stage, Position, ActionType,
        CFREngine,
        WinRateCalculator,
        SelfPlayEnv, StepResult,
    )
except ImportError as e:
    raise ImportError(
        "C++ core library not compiled for Python 3.10.\n"
        "Windows MSVC: cd cpp && mkdir build && cd build && "
        "cmake .. -G \"Visual Studio 17 2022\" -A x64 "
        "-DPYBIND11_PYTHON_VERSION=3.10 && "
        "cmake --build . --config Release\n"
        "Then: cd ../.. && pip install -e ."
    ) from e

__all__ = [
    'Card', 'create_deck', 'parse_protocol_cards',
    'HandEvaluator', 'HandResult',
    'GameState', 'ActionRecord', 'Stage', 'Position', 'ActionType',
    'CFREngine',
    'WinRateCalculator',
    'SelfPlayEnv', 'StepResult',
]
