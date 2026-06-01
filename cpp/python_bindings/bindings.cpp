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

PYBIND11_MODULE(_core, m) {
    m.doc() = "Poker AI C++ Core Library (Python 3.10 + CUDA 12.6)";

    py::class_<Card>(m, "Card")
        .def(py::init<uint8_t, uint8_t>())
        .def_readwrite("suit", &Card::suit)
        .def_readwrite("rank", &Card::rank)
        .def("index", &Card::index)
        .def("to_string", &Card::to_string)
        .def("to_protocol", &Card::to_protocol)
        .def("__repr__", &Card::to_string)
        .def("__eq__", &Card::operator==)
        .def("__hash__", &Card::index);

    m.def("create_deck", &create_deck, "Create a 52-card deck");
    m.def("parse_protocol_cards", &parse_protocol_cards, "Parse cards from protocol string");

    py::class_<HandResult>(m, "HandResult")
        .def_readwrite("rank", &HandResult::rank)
        .def_readwrite("kickers", &HandResult::kickers)
        .def("__gt__", &HandResult::operator>)
        .def("__eq__", &HandResult::operator==);

    py::class_<HandEvaluator>(m, "HandEvaluator")
        .def(py::init<>())
        .def("evaluate_7cards", &HandEvaluator::evaluate_7cards)
        .def("evaluate_batch", &HandEvaluator::evaluate_batch);

    py::enum_<Stage>(m, "Stage")
        .value("PREFLOP", Stage::PREFLOP)
        .value("FLOP", Stage::FLOP)
        .value("TURN", Stage::TURN)
        .value("RIVER", Stage::RIVER)
        .value("SHOWDOWN", Stage::SHOWDOWN);

    py::enum_<Position>(m, "Position")
        .value("SB", Position::SB)
        .value("BB", Position::BB)
        .value("BUTTON", Position::BUTTON);

    py::enum_<ActionType>(m, "ActionType")
        .value("FOLD", ActionType::FOLD)
        .value("CHECK", ActionType::CHECK)
        .value("CALL", ActionType::CALL)
        .value("BET", ActionType::BET)
        .value("RAISE", ActionType::RAISE)
        .value("ALLIN", ActionType::ALLIN);

    py::class_<ActionRecord>(m, "ActionRecord")
        .def_readwrite("stage", &ActionRecord::stage)
        .def_readwrite("action", &ActionRecord::action)
        .def_readwrite("amount", &ActionRecord::amount)
        .def_readwrite("is_opponent", &ActionRecord::is_opponent);

    py::class_<GameState>(m, "GameState")
        .def(py::init<>())
        .def_readwrite("my_cards", &GameState::my_cards)
        .def_readwrite("community_cards", &GameState::community_cards)
        .def_readwrite("num_community", &GameState::num_community)
        .def_readwrite("my_position", &GameState::my_position)
        .def_readwrite("stage", &GameState::stage)
        .def_readwrite("my_chips", &GameState::my_chips)
        .def_readwrite("opponent_chips", &GameState::opponent_chips)
        .def_readwrite("pot", &GameState::pot)
        .def_readwrite("to_call", &GameState::to_call)
        .def_readwrite("history", &GameState::history)
        .def_readwrite("opponent_vpip", &GameState::opponent_vpip)
        .def_readwrite("opponent_pfr", &GameState::opponent_pfr)
        .def_readwrite("opponent_af", &GameState::opponent_af)
        .def("get_stage_history", &GameState::get_stage_history)
        .def("is_first_action", &GameState::is_first_action);

    py::class_<CFREngine>(m, "CFREngine")
        .def(py::init<>())
        .def("make_info_set_key", &CFREngine::make_info_set_key)
        .def("get_strategy", &CFREngine::get_strategy)
        .def("get_average_strategy", &CFREngine::get_average_strategy)
        .def("train_iteration", &CFREngine::train_iteration)
        .def("train_iterations", &CFREngine::train_iterations)
        .def("save_strategy", &CFREngine::save_strategy)
        .def("load_strategy", &CFREngine::load_strategy)
        .def("num_info_sets", &CFREngine::num_info_sets);

    py::class_<WinRateCalculator>(m, "WinRateCalculator")
        .def(py::init<>())
        .def("calculate", &WinRateCalculator::calculate,
             py::arg("my_cards"), py::arg("community"), py::arg("num_opponents"), py::arg("n_simulations"))
        .def("calculate_batch", &WinRateCalculator::calculate_batch,
             py::arg("my_cards_list"), py::arg("community_list"), py::arg("num_opponents"), py::arg("n_simulations_each"));

    py::class_<SelfPlayEnv>(m, "SelfPlayEnv")
        .def(py::init<>())
        .def("reset", &SelfPlayEnv::reset, py::arg("seed") = 0)
        .def("step", &SelfPlayEnv::step)
        .def("step_batch", &SelfPlayEnv::step_batch)
        .def("set_opponent_policy", &SelfPlayEnv::set_opponent_policy);

    py::class_<SelfPlayEnv::StepResult>(m, "StepResult")
        .def_readwrite("state", &SelfPlayEnv::StepResult::state)
        .def_readwrite("reward", &SelfPlayEnv::StepResult::reward)
        .def_readwrite("done", &SelfPlayEnv::StepResult::done);
}
