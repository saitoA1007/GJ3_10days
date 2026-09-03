#pragma once
#include <algorithm>
#include "Vector2.h"
#include "Vector3.h"
#include "Quaternion.h"

namespace GameEngine {
	
	/// <summary>
	/// 線形補間
	/// </summary>
	/// <param name="start">始点</param>
	/// <param name="end">終点</param>
	/// <param name="timer">時間</param>
	/// <returns></returns>
	float Lerp(const float& start, const float& end, const float& t);
	Vector2 Lerp(Vector2 start, Vector2 end, float t);
	Vector3 Lerp(const Vector3& start, const Vector3& end, const float& t);
	Quaternion Lerp(const Quaternion& start, const Quaternion& end, const float& t);

	// イージングイン
	float EaseIn(float t);

	// イージングアウト
	float EaseOut(float t);

	// イージングインアウト
	float EaseInOut(float t);

	// 球面線形補間
	Quaternion Slerp(const Quaternion& q0, const Quaternion& q1, float t);
	Vector3 Slerp(const Vector3& start, const Vector3& end, float t);

	enum class EaseType {

		// 等速

		Linear,

		// Quadratic

		EaseInQuad,
		EaseOutQuad,
		EaseInOutQuad,
		EaseOutInQuad,

		// Cubic

		EaseInCubic,
		EaseOutCubic,
		EaseInOutCubic,
		EaseOutInCubic,

		// Quartic

		EaseInQuart,
		EaseOutQuart,
		EaseInOutQuart,
		EaseOutInQuart,

		// Quintic

		EaseInQuint,
		EaseOutQuint,
		EaseInOutQuint,
		EaseOutInQuint,

		// Sine

		EaseInSine,
		EaseOutSine,
		EaseInOutSine,
		EaseOutInSine,

		// Exponential

		EaseInExpo,
		EaseOutExpo,
		EaseInOutExpo,
		EaseOutInExpo,

		// Circular

		EaseInCirc,
		EaseOutCirc,
		EaseInOutCirc,
		EaseOutInCirc,

		// Back

		EaseInBack,
		EaseOutBack,
		EaseInOutBack,
		EaseOutInBack,

		// Elastic

		EaseInElastic,
		EaseOutElastic,
		EaseInOutElastic,
		EaseOutInElastic,

		// Bounce

		EaseInBounce,
		EaseOutBounce,
		EaseInOutBounce,
		EaseOutInBounce
	};

	/// @brief イージング関数を適用
	/// @param t 進行状況 0.0～1.0
	/// @param type イージングタイプ
	/// @return イージング適用済み値 0.0～1.0
	float Apply(float t, EaseType type);

	/// @brief 任意型の補間 Start -> End
	/// @param start      開始ベクトル/値
	/// @param end       終了ベクトル/値
	/// @param t            進行状況 0.0～1.0
	/// @param goType イージングタイプ
	template<typename T>
	T lerp(const T& start, const T& end, float t, EaseType type = EaseType::Linear) {
		float easedT = Apply(t, type);
		return T(start + (end - start) * easedT);
	}

	/// @brief 任意型の補間、行って帰ってくる
	/// @param start         開始ベクトル/値
	/// @param end          中間ベクトル/値
	/// @param t               進行状況 0.0～1.0
	/// @param goType    行きのイージング
	/// @param backType 帰りのイージング
	template<typename T>
	T lerp_RoundTrip(const T& start, const T& end, float t,
		EaseType goType = EaseType::Linear, EaseType backType = EaseType::Linear) {

		// 0〜1 に Clamp
		t = std::clamp(t, 0.0f, 1.0f);

		if (t < 0.5f) {
			// 行き：start → end
			float normalizedT = t * 2.0f;           // 0〜0.5 → 0〜1
			return lerp<T>(start, end, normalizedT, goType);
		} else {
			// 帰り：end → start
			float normalizedT = (t - 0.5f) * 2.0f;  // 0.5〜1 → 0〜1
			return lerp<T>(end, start, normalizedT, backType);
		}
	}
}

