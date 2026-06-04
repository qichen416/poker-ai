#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include <pybind11/functional.h>
#include <pybind11/numpy.h>
#include "poker/card.h"
#include "poker/hand_evaluator.h"
#include "poker/game_state.h"
#include "poker/cfr_engine.h"
#include "poker/win_rate.h"
#include "poker/environment.h"

namespace py = pybind11;
using namespace poker;

// 定义 Python 模块 _core，提供 C++ 侧扑克 AI 核心功能的绑定
PYBIND11_MODULE(_core, m) {
    m.doc() = "Poker AI C++ Core Library (Python 3.10 + CUDA 12.6)";

    // ---------- Card：扑克牌 ----------
    py::class_<Card>(m, "Card")
        .def(py::init<uint8_t, uint8_t>())           // 构造：花色，点数（0-3, 0-12）
        .def_readwrite("suit", &Card::suit)          // 花色
        .def_readwrite("rank", &Card::rank)          // 点数
        .def("index", &Card::index)                  // 唯一索引 0-51
        .def("to_string", &Card::to_string)          // 转为可读字符串，如 "Ah"
        .def("to_protocol", &Card::to_protocol)      // 转为通信协议格式字符串
        .def("__repr__", &Card::to_string)           // Python repr 支持
        .def("__eq__", &Card::operator==)            // 比较相等
        .def("__hash__", &Card::index);              // 以索引作为哈希值

    // 全局函数：创建完整 52 张牌的牌堆
    m.def("create_deck", &create_deck, "Create a 52-card deck");
    // 全局函数：从协议字符串解析多张牌
    m.def("parse_protocol_cards", &parse_protocol_cards, "Parse cards from protocol string");

    // ---------- HandResult：牌型比较结果 ----------
    py::class_<HandResult>(m, "HandResult")
        .def_readwrite("rank", &HandResult::rank)       // 牌型等级（高牌、一对等）
        .def_readwrite("kickers", &HandResult::kickers) // 踢脚牌列表
        .def("__gt__", &HandResult::operator>)          // > 比较
        .def("__eq__", &HandResult::operator==);        // == 比较

    // ---------- HandEvaluator：手牌评估器 ----------
    py::class_<HandEvaluator>(m, "HandEvaluator")
        .def(py::init<>())
        .def("evaluate_7cards", &HandEvaluator::evaluate_7cards)   // 7张牌中找出最佳5张并评估
        .def("evaluate_batch", &HandEvaluator::evaluate_batch);    // 批量评估

    // ---------- 游戏阶段枚举 ----------
    py::enum_<Stage>(m, "Stage")
        .value("PREFLOP", Stage::PREFLOP)     // 翻牌前
        .value("FLOP", Stage::FLOP)           // 翻牌
        .value("TURN", Stage::TURN)           // 转牌
        .value("RIVER", Stage::RIVER)         // 河牌
        .value("SHOWDOWN", Stage::SHOWDOWN);  // 摊牌

    // ---------- 位置枚举 ----------
    py::enum_<Position>(m, "Position")
        .value("SB", Position::SB)            // 小盲
        .value("BB", Position::BB)            // 大盲
        .value("BUTTON", Position::BUTTON);   // 按钮位

    // ---------- 动作类型枚举 ----------
    py::enum_<ActionType>(m, "ActionType")
        .value("FOLD", ActionType::FOLD)      // 弃牌
        .value("CHECK", ActionType::CHECK)    // 过牌
        .value("CALL", ActionType::CALL)      // 跟注
        .value("BET", ActionType::BET)        // 下注
        .value("RAISE", ActionType::RAISE)    // 加注
        .value("ALLIN", ActionType::ALLIN);   // 全下

    // ---------- ActionRecord：单条动作记录 ----------
    py::class_<ActionRecord>(m, "ActionRecord")
        .def_readwrite("stage", &ActionRecord::stage)           // 发生阶段
        .def_readwrite("action", &ActionRecord::action)         // 动作类型
        .def_readwrite("amount", &ActionRecord::amount)         // 金额
        .def_readwrite("is_opponent", &ActionRecord::is_opponent); // 是否对手动作

    // ---------- GameState：单局游戏状态 ----------
    py::class_<GameState>(m, "GameState")
        .def(py::init<>())
        .def_readwrite("my_cards", &GameState::my_cards)                 // 我方手牌
        .def_readwrite("community_cards", &GameState::community_cards)   // 公共牌
        .def_readwrite("num_community", &GameState::num_community)       // 公共牌数量
        .def_readwrite("my_position", &GameState::my_position)           // 我方位置
        .def_readwrite("stage", &GameState::stage)                       // 当前阶段
        .def_readwrite("my_chips", &GameState::my_chips)                 // 我方筹码
        .def_readwrite("opponent_chips", &GameState::opponent_chips)     // 对手筹码
        .def_readwrite("pot", &GameState::pot)                           // 底池
        .def_readwrite("to_call", &GameState::to_call)                   // 需要跟注的金额
        .def_readwrite("history", &GameState::history)                   // 历史动作记录
        .def_readwrite("opponent_vpip", &GameState::opponent_vpip)       // 对手 VPIP 统计
        .def_readwrite("opponent_pfr", &GameState::opponent_pfr)         // 对手 PFR 统计
        .def_readwrite("opponent_af", &GameState::opponent_af)           // 对手 AF 统计
        .def("get_stage_history", &GameState::get_stage_history)         // 获取当前阶段动作序列
        .def("is_first_action", &GameState::is_first_action);            // 是否为阶段内首次动作

    // ---------- CFREngine：CFR 训练引擎 ----------
    py::class_<CFREngine>(m, "CFREngine")
        .def(py::init<>())
        .def("make_info_set_key", &CFREngine::make_info_set_key)       // 生成信息集唯一键
        .def("get_strategy", &CFREngine::get_strategy)                 // 获取当前策略（未平均）
        .def("get_average_strategy", &CFREngine::get_average_strategy) // 获取平均策略
        .def("train_iteration", &CFREngine::train_iteration)           // 单次迭代训练
        .def("train_iterations", &CFREngine::train_iterations)         // 多次迭代训练
        .def("save_strategy", &CFREngine::save_strategy)               // 保存策略到文件
        .def("load_strategy", &CFREngine::load_strategy)               // 从文件加载策略
        .def("num_info_sets", &CFREngine::num_info_sets);              // 信息集总数

    // ---------- WinRateCalculator：胜率计算器 ----------
    py::class_<WinRateCalculator>(m, "WinRateCalculator")
        .def(py::init<>())
        // 通过蒙特卡洛模拟计算胜率（我的牌，公共牌，对手数，模拟次数）
        .def("calculate", &WinRateCalculator::calculate,
             py::arg("my_cards"), py::arg("community"),
             py::arg("num_opponents"), py::arg("n_simulations"))
        // 批量胜率计算
        .def("calculate_batch", &WinRateCalculator::calculate_batch,
             py::arg("my_cards_list"), py::arg("community_list"),
             py::arg("num_opponents"), py::arg("n_simulations_each"));

    // ---------- SelfPlayEnv：自对弈环境 ----------
    py::class_<SelfPlayEnv>(m, "SelfPlayEnv")
        .def(py::init<>())
        .def("reset", &SelfPlayEnv::reset, py::arg("seed") = 0)          // 重置环境，可选随机种子
        .def("step", &SelfPlayEnv::step)                                 // 执行一步
        .def("step_batch", &SelfPlayEnv::step_batch)                     // 批量执行步骤
        .def("set_opponent_policy", &SelfPlayEnv::set_opponent_policy);  // 设置对手策略

    // ---------- StepResult：单步结果 ----------
    py::class_<SelfPlayEnv::StepResult>(m, "StepResult")
        .def_readwrite("state", &SelfPlayEnv::StepResult::state)   // 新状态
        .def_readwrite("reward", &SelfPlayEnv::StepResult::reward) // 奖励值
        .def_readwrite("done", &SelfPlayEnv::StepResult::done);    // 是否终局
}