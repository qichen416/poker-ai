# Poker AI (C++ + Python Hybrid) - Python 3.10 + CUDA 12.6 + Windows + VSCode

## 环境要求（全队统一！）

| 组件                        | 版本                   | 说明                                |
| --------------------------- | ---------------------- | ----------------------------------- |
| **Python**            | **3.10.x**       | 全队统一，PyTorch长期支持稳定版     |
| **PyTorch**           | **2.12.0+cu126** | CUDA 12.6官方支持                   |
| **CUDA Toolkit**      | **12.6**         | NVIDIA驱动需>=525.60.13             |
| **NVIDIA驱动**        | **>=525.60**     | 支持CUDA 12.6                       |
| **numpy**             | **1.24.3**       | Python 3.10稳定版，<2.0避免兼容问题 |
| **stable-baselines3** | **>=2.0.0**      | Python 3.10完全兼容                 |
| **pybind11**          | **>=2.10.0**     | Python 3.10完全兼容                 |
| **CMake**             | **>=3.20**       | 标准版本即可                        |
| **编译器**            | **MSVC 2022**   | 全队必须统一                        |

## 快速开始

```powershell
# 1. 创建conda环境（Python 3.10）
conda create -n poker python=3.10
conda activate poker

# 2. 安装CUDA 12.6对应PyTorch（关键！）
pip3 install torch torchvision --index-url https://download.pytorch.org/whl/cu126

# 3. 安装其他依赖
pip install -r requirements.txt

# 4. 验证CUDA可用
python -c "import torch; print(f'PyTorch {torch.__version__}'); print(f'CUDA available: {torch.cuda.is_available()}'); print(f'CUDA version: {torch.version.cuda}')"
# 期望输出: CUDA available: True, CUDA version: 12.6

# 5. 编译C++核心
cd cpp
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
cd ../..

# 6. 安装Python包（链接编译好的.pyd）
pip install -e .

# 7. 验证
python -c "from poker_core import Card; print(Card(0,12).to_string())"
# 输出: Ah

# 8. 运行测试
pytest tests/python/ -v
```

## CUDA 12.6 特别注意事项

1. **PyTorch安装**: 必须使用 `--extra-index-url https://download.pytorch.org/whl/cu126`，默认pip安装的是CPU版
2. **NVIDIA驱动**: 4060笔记本需驱动>=525.60.13，建议去NVIDIA官网更新到最新
3. **CUDA Toolkit**: 不需要完整安装CUDA Toolkit，PyTorch自带runtime。如需编译其他CUDA扩展，可装CUDA 12.6 Toolkit
4. **numpy版本**: 使用1.24.3（<2.0），避免与某些旧版C扩展的兼容问题
5. **验证CUDA**: 每次环境搭建后必须运行 `torch.cuda.is_available()` 验证

## 启动说明

**1. 必须从 Anaconda Prompt 启动 VSCode**

双击 VSCode 图标会导致 CMake 找不到 Python 3.10！

```powershell
conda activate poker
code E:\poker\poker-ai-cu126-py310  //你自己的路径
```

## 项目结构

```
poker-ai/                          ← 项目根（总入口）
│
├── README.md                      ← 项目说明 + 环境安装命令（含 CUDA 12.6 特殊指令）
├── requirements.txt               ← Python 依赖清单（注意：PyTorch 不走这里安装）
├── setup.py                       ← 【关键】把 C++ 编译成 Python 能 import 的 .pyd 包
├── .gitignore                     ← Git 忽略规则（编译产物、数据文件、日志）
├── .gitattributes                 ← Git-LFS 配置（.pkl/.npy/.pt 大文件）
│
├── cpp/                           ← 【C++ 核心库】所有计算密集型算法
│   ├── include/poker/             ← 头文件（.h）= C++ 的"接口契约"
│   │   ├── card.h                 ← Card 类定义：suit(0-3), rank(0-12), to_string()
│   │   ├── hand_evaluator.h       ← 7张牌评估器：输入7张牌 → 输出(牌型等级, 踢脚)
│   │   ├── game_state.h           ← GameState 结构体：牌、筹码、位置、历史记录
│   │   ├── cfr_engine.h           ← MCCFR 引擎：信息集 Key → 遗憾值表 → 策略概率
│   │   ├── win_rate.h             ← 蒙特卡洛胜率：输入手牌+公共牌 → 模拟1万次 → 胜率
│   │   └── environment.h          ← 自博弈环境：发牌、执行动作、判定胜负
│   │
│   ├── src/                       ← 实现文件（.cpp）= 算法的"真正代码"
│   │   ├── card.cpp               ← Card::to_string() 等函数实现
│   │   ├── hand_evaluator.cpp     ← 21种5张组合枚举 + 牌型判断逻辑
│   │   ├── game_state.cpp         ← get_stage_history() 等成员函数
│   │   ├── cfr_engine.cpp         ← Regret Matching + MCCFR 遍历逻辑
│   │   ├── win_rate.cpp           ← 多线程蒙特卡洛采样 + 胜负比较
│   │   └── environment.cpp        ← 德州扑克规则引擎（发牌/下注/比大小）
│   │
│   ├── python_bindings/           ← 【pybind11 胶水层】唯一和 Python 交互的 C++ 文件
│   │   └── bindings.cpp           ← 注册：Card 类、GameState 结构、CFREngine 类...
│   │                                让 Python 可以 `from poker_core import Card`
│   │
│   ├── tests/                     ← C++ 单元测试（纯 C++，不依赖 Python）
│   │   ├── test_hand_eval.cpp     ← 测试：高牌/对子/同花顺评估是否正确
│   │   └── test_cfr.cpp           ← 测试：策略概率和是否为 1.0
│   │
│   └── CMakeLists.txt             ← C++ 构建脚本：源文件列表 → 编译成 .lib + .exe
│
├── poker_core/                    ← 【Python 包装层】C++ 库的"门面"
│   └── __init__.py                ← `from poker_core._core import Card`
│                                    如果 C++ 没编译，给出清晰报错提示
│
├── engine/                        ← 【Python】通信 + 决策（调用 C++ 核心）
│   ├── client.py                  ← Socket 客户端：连接比赛平台 → 收消息/发动作
│   ├── parser.py                  ← 协议解析："preflop|BIGBLIND|<2,3>" → 填充 GameState
│   ├── fusion.py                  ← 决策融合层：翻前查 CFR / 翻后 CFR+DRL+对手偏移
│   └── main.py                    ← 程序入口：连接 → 循环(收状态 → 决策 → 发动作)
│
├── drl/                           ← 【Python】PyTorch 深度强化学习
│   ├── network.py                 ← PokerPolicy：CNN(牌面) + GRU(历史) + FC(输出)
│   ├── environment.py             ← 自博弈环境封装（供 PPO 训练生成数据）
│   ├── train.py                   ← PPO 训练脚本：收集轨迹 → 计算优势 → 更新网络
│   └── infer.py                   ← 推理接口：加载模型权重 → 输入 GameState → 输出动作概率
│
├── eval/                          ← 【Python】评估 + 对手建模
│   ├── hand_eval.py               ← Python 端包装（实际计算在 C++ HandEvaluator）
│   ├── opponent_model.py          ← 统计 VPIP/PFR/AF → 根据对手松紧调整诈唬频率
│   └── elo.py                     ← Elo 评分：两个 AI 版本对打 → 计算相对强度
│
├── shared/                        ← 【Python】常量 + 工具（与 C++ 头文件逻辑一致）
│   ├── constants.py               ← INITIAL_CHIPS=20000, DECISION_TIMEOUT=5.0
│   └── utils.py                   ← parse_action_string("raise 200") → ("raise", 200)
│
├── configs/                       ← 【YAML】超参配置文件（C++ 和 Python 共用）
│   ├── cfr.yaml                   ← MCCFR 迭代次数 / 抽象桶数 / 策略文件路径
│   ├── drl.yaml                   ← PPO 学习率 / batch_size / 网络结构 / 模型保存路径
│   └── engine.yaml                ← 融合权重 α / 决策时限 / 各模块文件路径
│
├── tests/                         ← 测试
│   ├── mock_server.py             ← 模拟比赛平台：发牌 → 等 AI 动作 → 继续发牌
│   └── python/                    ← Python 集成测试
│       └── test_cpp_binding.py    ← 验证：C++ Card / HandEvaluator / CFREngine 能被 Python 调用
│
├── data/                          ← 策略文件（Resilio Sync / Git-LFS 同步，不在 Git 仓库里）
│   ├── preflop/                   ← 翻前 GTO 策略表（169 种起手牌 × 位置 → 动作概率）
│   ├── cfr_postflop/            ← 翻后 CFR 基线策略（信息集 → 动作概率）
│   ├── drl_checkpoints/         ← DRL 模型权重（.pt 文件，每 2 小时保存一次）
│   └── opponent_db/             ← 对手历史数据（每局对手动作 → VPIP/PFR/AF 统计）
│
└── .vscode/                       ← 【VSCode 配置】开箱即用
    ├── extensions.json            ← 推荐插件：C++ / Python / CMake / GitLens
    ├── c_cpp_properties.json    ← IntelliSense：告诉 VSCode 头文件在哪里（含 pybind11）
    ├── tasks.json                 ← Ctrl+Shift+B 一键编译 C++ / 安装 Python 包
    ├── launch.json                ← F5 调试：Python 主引擎 / Mock 服务器 / C++ 单元测试
    └── settings.json              ← Python 解释器路径 / CMake 配置目录 
```
