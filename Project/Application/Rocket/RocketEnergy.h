#pragma once

#include <cstdint>

/// @brief エネルギーが変化した理由
enum class EnergyChangeReason : uint8_t 
{
	Delivery,       // 配達による増加
	UnitAllocation, // ユニット割り当てによる消費
	EnemyHit,       // 敵の攻撃による消費
	Debug,          // デバッグによる増減
	Reset,          // リセットによる増減
};

/// @brief 一度のエネルギー増減結果。
struct EnergyChange
{
	int32_t before = 0; // 変更前の保有量
	int32_t amount = 0; // 実際の増減量。消費時は負数
	int32_t after = 0;  // 変更後の保有量
	EnergyChangeReason reason = EnergyChangeReason::Delivery; // 変更元の処理

	/// @brief 実際に値が変化したか判定する。
	/// @return 増減量が0以外ならtrue。
	bool Changed() const { return amount != 0; }
};

/// @brief ロケットが保有するエネルギーの値だけを管理
class RocketEnergy final 
{
public:
	/// @brief 負数を0へ補正して初期値を設定する
	explicit RocketEnergy(int32_t initialEnergy = 0);

	/// @brief 保有量を指定値へ置き換える
	EnergyChange Reset(int32_t energy = 0);

	/// @brief 正の値だけを保有量へ加算
	EnergyChange Add(int32_t amount, EnergyChangeReason reason);

	/// @brief 保有量を超えない範囲でエネルギーを消費
	EnergyChange ConsumeUpTo(int32_t requestedAmount, EnergyChangeReason reason);

	/// @brief 現在の保有量を取得する
	int32_t GetCurrent() const { return current_; }

	/// @brief エネルギーが空か判定する
	bool IsEmpty() const { return current_ == 0; }

private:
	int32_t current_ = 0; // 現在の保有量。常に0以上
};

