#pragma once
#include <cstdint>

// プレイヤー陣営
static inline const uint32_t kCollisionAttributePlayer = 0b1;
// ピクミ陣営
static inline const uint32_t kCollisionAttributePikumi = 0b1;
// 敵陣営
static inline const uint32_t kCollisionAttributeEnemy = 0b1 << 1;
// 地形陣営
static inline const uint32_t kCollisionAttributeTerrain = 0b1 << 2;

/// <summary>
/// 当たり判定がもつID
/// </summary>
enum class CollisionTypeID : uint32_t {
	kDefault, // 通常
	kPlayer,  // プレイヤー
	kPikumi,  // ピクミ
	kEnemy,	  // 敵
	kWall,    // 壁
	kGround,  // 地面
	kIceFall, // つらら
	kWind,    // ボスの風攻撃
	kHeart,   // 回復
	kBoundaryWall, // 移動範囲制限用の壁
};
