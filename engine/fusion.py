import time
import random
from typing import Dict
from poker_core import GameState, CFREngine, WinRateCalculator, ActionType, Stage, Position
from shared.constants import DECISION_TIMEOUT   # 决策超时阈值（秒）
from drl import DRLInferencer                  # 深度强化学习推理器
from eval.opponent_model import OpponentModel  # 对手建模模块

class DecisionEngine:
    """
    扑克决策引擎，综合 CFR（反事实遗憾最小化）、深度强化学习和对手模型，
    在不同的游戏阶段生成行动决策。
    """

    def __init__(self):
        """
        初始化决策引擎，加载各个子模块并设定混合策略权重。
        """
        self.cfr_engine = CFREngine()               # CFR 策略求解器，提供信息集平均策略
        self.win_calc = WinRateCalculator()         # 胜率计算器（当前未直接使用，可由外部调用）
        self.drl = DRLInferencer()                  # 深度强化学习模型，输出动作概率
        self.opponent = OpponentModel()             # 对手行为建模，用于动态调整（当前未直接使用）
        self.alpha = 0.5                            # 混合权重：CFR 权重 (1-alpha)，DRL 权重 alpha

    def make_decision(self, game_state: GameState) -> str:
        """
        根据当前游戏状态生成一个决策动作字符串。

        决策逻辑：
        - 翻牌前使用纯 CFR 策略。
        - 翻牌后融合 CFR 和 DRL 的策略概率。
        - 整个决策过程有超时保护，若接近超时则兜底返回 "call"。

        :param game_state: 当前游戏状态对象
        :return: 动作字符串，例如 "fold", "call", "raise 500" 等
        """
        start_time = time.time()

        # 根据游戏阶段选择不同的决策函数
        if game_state.stage == Stage.PREFLOP:
            action = self._preflop_decision(game_state)
        else:
            action = self._postflop_decision(game_state)

        elapsed = time.time() - start_time
        # 如果决策耗时超过 90% 的超时阈值，打印警告并使用安全动作
        if elapsed > DECISION_TIMEOUT * 0.9:
            print("WARNING: timeout (%.2fs), fallback to call" % elapsed)
            return "call"
        return action

    def _preflop_decision(self, state: GameState) -> str:
        """
        翻牌前决策：仅使用 CFR 平均策略，不考虑深度强化学习。

        :param state: 当前游戏状态
        :return: 动作字符串
        """
        # 构建信息集 key，用于查询 CFR 策略表
        key = self.cfr_engine.make_info_set_key(state)
        # 获取该信息集下的动作概率分布
        probs = self.cfr_engine.get_average_strategy(key)
        # 按概率采样动作并返回
        return self._sample_action(probs, state)

    def _postflop_decision(self, state: GameState) -> str:
        """
        翻牌后决策：融合 CFR 策略和 DRL 策略，并按权重混合后采样。

        :param state: 当前游戏状态
        :return: 动作字符串
        """
        # 获取 CFR 策略概率
        key = self.cfr_engine.make_info_set_key(state)
        cfr_probs = self.cfr_engine.get_average_strategy(key)
        # 获取 DRL 模型给出的动作概率（通常基于局面特征）
        drl_probs = self.drl.get_action_probs_c(state)
        # 线性加权合并两种概率
        merged = self._merge_probs(cfr_probs, drl_probs, self.alpha)
        return self._sample_action(merged, state)

    def _merge_probs(self, cfr, drl, alpha):
        """
        将两个来源的动作概率按权重线性混合，并归一化。

        支持 CFR 输出为列表/元组或字典，DRL 输出为列表或字典。
        预定义的动作顺序为 ["fold", "check", "call", "bet", "raise", "allin"]。

        :param cfr: CFR 策略概率（list/tuple 或 dict）
        :param drl: DRL 策略概率（list 或 dict）
        :param alpha: DRL 策略的混合权重
        :return: 归一化后的动作概率字典，键为动作名，值为概率
        """
        actions = ["fold", "check", "call", "bet", "raise", "allin"]
        merged = {}
        for i, a in enumerate(actions):
            # 从 CFR 概率中提取动作 a 的概率（兼容列表/字典格式）
            c = cfr[i] if isinstance(cfr, (list, tuple)) else cfr.get(a, 0.0)
            # 从 DRL 概率中提取对应概率
            d = drl.get(a, 0.0) if isinstance(drl, dict) else drl[i]
            merged[a] = (1 - alpha) * c + alpha * d

        total = sum(merged.values())
        # 避免除零，归一化
        return {k: v/total for k, v in merged.items()} if total > 0 else merged

    def _sample_action(self, probs, state: GameState) -> str:
        """
        根据动作概率分布随机采样一个动作，对于下注/加注动作会附加一个下注额。

        下注额目前简单取 max(底池大小, 100)，可根据实际策略替换为更复杂的下注尺度模型。

        :param probs: 动作名到概率的映射字典
        :param state: 当前游戏状态（用于获取底池大小等信息）
        :return: 动作字符串，如 "fold", "call", "raise 200"
        """
        actions = list(probs.keys())
        weights = [probs[a] for a in actions]
        # 按权重随机选取动作
        action = random.choices(actions, weights=weights, k=1)[0]

        # 如果是激进动作，生成下注/加注金额
        if action in ['raise', 'bet']:
            amount = max(state.pot, 100)          # 简单设定：至少等于底池，且不低于 100
            return "%s %d" % (action, amount)
        return action