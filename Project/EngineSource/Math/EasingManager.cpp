#include "EasingManager.h"
#include <cmath>
#include <numbers>
#include "MyMath.h"

namespace GameEngine {

	float Lerp(const float& start, const float& end, const float& t) {
		return start + t * (end - start);
	}

	Vector2 Lerp(Vector2 start, Vector2 end, float t) {
		return Vector2(start.x + t * (end.x - start.x), start.y + t * (end.y - start.y));
	}

	Vector3 Lerp(const Vector3& start, const Vector3& end, const float& t) {
		return Vector3(start.x + t * (end.x - start.x), start.y + t * (end.y - start.y), start.z + t * (end.z - start.z));
	}

	Quaternion Lerp(const Quaternion& start, const Quaternion& end, const float& t) {
		return Quaternion(start.x + t * (end.x - start.x), start.y + t * (end.y - start.y), start.z + t * (end.z - start.z), start.w + t * (end.w - start.w));
	}

	float EaseIn(float t) {
		return t * t;
	}

	float EaseOut(float t) {
		return 1.0f - std::powf(1.0f - t, 2.0f);
	}

	float EaseInOut(float t) {
		if (t < 0.5f) {
			return 2.0f * t * t;
		} else {
			return 1.0f - std::powf(-2.0f * t + 2.0f, 2.0f) / 2.0f;
		}
	}

	Quaternion Slerp(const Quaternion& q0, const Quaternion& q1, float t) {
		Quaternion q1Copy = q1;
		float dot = Math::Dot(q0, q1);
		// 四元数の符号が逆だと最短経路で補間されないので反転
		if (dot < 0.0f) {
			dot = -dot;
			q1Copy = -q1;
		}
		// 内積が1,0fを超えないようにする
		dot = std::clamp(dot, -1.0f, 1.0f);

		if (dot > 0.9999f) {
			Quaternion result = Lerp(q0, q1Copy, t);
			result.Normalize();
			return result;
		}

		// なす角
		float theta = std::acosf(dot);
		float sinTheta = std::sinf(theta);
		// 補間係数を求める
		float scale0 = std::sinf((1.0f - t) * theta) / sinTheta;
		float scale1 = std::sinf(t * theta) / sinTheta;
		return scale0 * q0 + scale1 * q1Copy;
	}

	Vector3 Slerp(const Vector3& start, const Vector3& end, float t) {
		// 始点と終点の長さを取得
		float startMag = start.Length();
		float endMag = end.Length();

		// 正規化
		Vector3 startNorm = start;
		Vector3 endNorm = end;
		if (startMag > 1e-6f) {
			startNorm /= startMag;
		} else {
			startNorm = Vector3(0, 0, 1);
		};

		if (endMag > 1e-6f) {
			endNorm /= endMag;
		} else {
			endNorm = Vector3(0, 0, 1);
		};

		// 内積を求める
		float dot = Math::Dot(startNorm, endNorm);
		dot = std::clamp(dot, -1.0f, 1.0f);

		Vector3 interpVec;

		if (dot > 0.9995f) {
			// 線形補間して正規化
			interpVec = Math::Normalize(Lerp(startNorm, endNorm, t));
		} else if (dot < -0.9995f) {
			Vector3 ortho = Math::Cross(startNorm, Vector3(0.0f, 1.0f, 0.0f));
			if (ortho.Length() < 0.01f) {
				ortho = Math::Cross(startNorm, Vector3(1.0f, 0.0f, 0.0f));
			}
			// 回転の基準となる垂直ベクトルを正規化
			ortho.Normalize();
			float theta = PI * t;
			interpVec = startNorm * std::cosf(theta) + ortho * std::sinf(theta);

		} else {
			// Slerpの計算
			float theta = std::acosf(dot);
			float sinTheta = std::sinf(theta);

			float sinThetaFrom = std::sinf((1.0f - t) * theta);
			float sinThetaTo = std::sinf(t * theta);

			interpVec = (startNorm * sinThetaFrom + endNorm * sinThetaTo) / sinTheta;
		}

		// 長さの線形補間
		float magnitudeLerp = Lerp(startMag, endMag, t);
		return interpVec * magnitudeLerp;
	}

	float Apply(float t, EaseType type) {
		// tを0.0～1.0の範囲にClamp
		t = std::clamp(t, 0.0f, 1.0f);

		switch (type) {
		case EaseType::Linear:
			return t;

		case EaseType::EaseInQuad:
			return t * t;

		case EaseType::EaseOutQuad:
			return 1.0f - (1.0f - t) * (1.0f - t);

		case EaseType::EaseInOutQuad:
			if (t < 0.5f) {
				return 2.0f * t * t;
			} else {
				return 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
			}

		case EaseType::EaseOutInQuad:
			if (t < 0.5f) {
				return 0.5f * (1.0f - (1.0f - 2.0f * t) * (1.0f - 2.0f * t));
			} else {
				return 0.5f + 0.5f * (2.0f * t - 1.0f) * (2.0f * t - 1.0f);
			}

		case EaseType::EaseInCubic:
			return t * t * t;

		case EaseType::EaseOutCubic:
			return 1.0f - std::pow(1.0f - t, 3.0f);

		case EaseType::EaseInOutCubic:
			if (t < 0.5f) {
				return 4.0f * t * t * t;
			} else {
				return 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
			}

		case EaseType::EaseOutInCubic:
			if (t < 0.5f) {
				return 0.5f * (1.0f - std::pow(1.0f - 2.0f * t, 3.0f));
			} else {
				return 0.5f + 0.5f * std::pow(2.0f * t - 1.0f, 3.0f);
			}

		case EaseType::EaseInQuart:
			return t * t * t * t;

		case EaseType::EaseOutQuart:
			return 1.0f - std::pow(1.0f - t, 4.0f);

		case EaseType::EaseInOutQuart:
			if (t < 0.5f) {
				return 8.0f * t * t * t * t;
			} else {
				return 1.0f - std::pow(-2.0f * t + 2.0f, 4.0f) / 2.0f;
			}

		case EaseType::EaseOutInQuart:
			if (t < 0.5f) {
				return 0.5f * (1.0f - std::pow(1.0f - 2.0f * t, 4.0f));
			} else {
				return 0.5f + 0.5f * std::pow(2.0f * t - 1.0f, 4.0f);
			}

		case EaseType::EaseInQuint:
			return t * t * t * t * t;

		case EaseType::EaseOutQuint:
			return 1.0f - std::pow(1.0f - t, 5.0f);

		case EaseType::EaseInOutQuint:
			if (t < 0.5f) {
				return 16.0f * t * t * t * t * t;
			} else {
				return 1.0f - std::pow(-2.0f * t + 2.0f, 5.0f) / 2.0f;
			}

		case EaseType::EaseOutInQuint:
			if (t < 0.5f) {
				return 0.5f * (1.0f - std::pow(1.0f - 2.0f * t, 5.0f));
			} else {
				return 0.5f + 0.5f * std::pow(2.0f * t - 1.0f, 5.0f);
			}

		case EaseType::EaseInSine:
			return 1.0f - std::cos((t * std::numbers::pi_v<float>) / 2.0f);

		case EaseType::EaseOutSine:
			return std::sin((t * std::numbers::pi_v<float>) / 2.0f);

		case EaseType::EaseInOutSine:
			return -(std::cos(std::numbers::pi_v<float> *t) - 1.0f) / 2.0f;

		case EaseType::EaseOutInSine:
			if (t < 0.5f) {
				return 0.5f * std::sin((2.0f * t * std::numbers::pi_v<float>) / 2.0f);
			} else {
				return 1.0f - 0.5f * std::cos(((2.0f * t - 1.0f) * std::numbers::pi_v<float>) / 2.0f);
			}

		case EaseType::EaseInExpo:
			return t == 0.0f ? 0.0f : std::pow(2.0f, 10.0f * (t - 1.0f));

		case EaseType::EaseOutExpo:
			return t == 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);

		case EaseType::EaseInOutExpo:
			if (t == 0.0f) return 0.0f;
			if (t == 1.0f) return 1.0f;
			if (t < 0.5f) {
				return std::pow(2.0f, 20.0f * t - 10.0f) / 2.0f;
			} else {
				return (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) / 2.0f;
			}

		case EaseType::EaseOutInExpo:
			if (t == 0.0f) return 0.0f;
			if (t == 1.0f) return 1.0f;
			if (t < 0.5f) {
				return 0.5f * (1.0f - std::pow(2.0f, -20.0f * t));
			} else {
				return 0.5f + 0.5f * std::pow(2.0f, 20.0f * (t - 1.0f));
			}

		case EaseType::EaseInCirc:
			return 1.0f - std::sqrt(1.0f - t * t);

		case EaseType::EaseOutCirc:
			return std::sqrt(1.0f - std::pow(t - 1.0f, 2.0f));

		case EaseType::EaseInOutCirc:
			if (t < 0.5f) {
				return (1.0f - std::sqrt(1.0f - std::pow(2.0f * t, 2.0f))) / 2.0f;
			} else {
				return (std::sqrt(1.0f - std::pow(-2.0f * t + 2.0f, 2.0f)) + 1.0f) / 2.0f;
			}

		case EaseType::EaseOutInCirc:
			if (t < 0.5f) {
				return 0.5f * std::sqrt(1.0f - std::pow(2.0f * t - 1.0f, 2.0f));
			} else {
				return 1.0f - 0.5f * std::sqrt(1.0f - std::pow(2.0f * t - 1.0f, 2.0f));
			}

		case EaseType::EaseInBack:
		{
			const float c1 = 1.70158f;
			const float c3 = c1 + 1.0f;
			return c3 * t * t * t - c1 * t * t;
		}

		case EaseType::EaseOutBack:
		{
			const float c1 = 1.70158f;
			const float c3 = c1 + 1.0f;
			return 1.0f + c3 * std::pow(t - 1.0f, 3.0f) + c1 * std::pow(t - 1.0f, 2.0f);
		}

		case EaseType::EaseInOutBack:
		{
			const float c1 = 1.70158f;
			const float c2 = c1 * 1.525f;
			if (t < 0.5f) {
				return (std::pow(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) / 2.0f;
			} else {
				return (std::pow(2.0f * t - 2.0f, 2.0f) * ((c2 + 1.0f) * (t * 2.0f - 2.0f) + c2) + 2.0f) / 2.0f;
			}
		}

		case EaseType::EaseOutInBack:
		{
			const float c1 = 1.70158f;
			const float c3 = c1 + 1.0f;
			if (t < 0.5f) {
				return 0.5f * (1.0f + c3 * std::pow(2.0f * t - 1.0f, 3.0f) + c1 * std::pow(2.0f * t - 1.0f, 2.0f));
			} else {
				return 0.5f + 0.5f * (c3 * std::pow(2.0f * t - 1.0f, 3.0f) - c1 * std::pow(2.0f * t - 1.0f, 2.0f));
			}
		}

		case EaseType::EaseInElastic:
		{
			const float c4 = (2.0f * std::numbers::pi_v<float>) / 3.0f;
			if (t == 0.0f) return 0.0f;
			if (t == 1.0f) return 1.0f;
			return -std::pow(2.0f, 10.0f * t - 10.0f) * std::sin((t * 10.0f - 10.75f) * c4);
		}

		case EaseType::EaseOutElastic:
		{
			const float c4 = (2.0f * std::numbers::pi_v<float>) / 3.0f;
			if (t == 0.0f) return 0.0f;
			if (t == 1.0f) return 1.0f;
			return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
		}

		case EaseType::EaseInOutElastic:
		{
			const float c5 = (2.0f * std::numbers::pi_v<float>) / 4.5f;
			if (t == 0.0f) return 0.0f;
			if (t == 1.0f) return 1.0f;
			if (t < 0.5f) {
				return -(std::pow(2.0f, 20.0f * t - 10.0f) * std::sin((20.0f * t - 11.125f) * c5)) / 2.0f;
			} else {
				return (std::pow(2.0f, -20.0f * t + 10.0f) * std::sin((20.0f * t - 11.125f) * c5)) / 2.0f + 1.0f;
			}
		}

		case EaseType::EaseOutInElastic:
		{
			const float c4 = (2.0f * std::numbers::pi_v<float>) / 3.0f;
			if (t == 0.0f) return 0.0f;
			if (t == 1.0f) return 1.0f;
			if (t < 0.5f) {
				return 0.5f * (std::pow(2.0f, -20.0f * t) * std::sin((20.0f * t - 0.75f) * c4) + 1.0f);
			} else {
				return 1.0f - 0.5f * std::pow(2.0f, 20.0f * (t - 1.0f)) * std::sin((20.0f * (t - 1.0f) - 10.75f) * c4);
			}
		}

		case EaseType::EaseInBounce:
			return 1.0f - Apply(1.0f - t, EaseType::EaseOutBounce);

		case EaseType::EaseOutBounce:
		{
			const float n1 = 7.5625f;
			const float d1 = 2.75f;

			if (t < 1.0f / d1) {
				return n1 * t * t;
			} else if (t < 2.0f / d1) {
				return n1 * (t -= 1.5f / d1) * t + 0.75f;
			} else if (t < 2.5f / d1) {
				return n1 * (t -= 2.25f / d1) * t + 0.9375f;
			} else {
				return n1 * (t -= 2.625f / d1) * t + 0.984375f;
			}
		}

		case EaseType::EaseInOutBounce:
			if (t < 0.5f) {
				return (1.0f - Apply(1.0f - 2.0f * t, EaseType::EaseOutBounce)) / 2.0f;
			} else {
				return (1.0f + Apply(2.0f * t - 1.0f, EaseType::EaseOutBounce)) / 2.0f;
			}

		case EaseType::EaseOutInBounce:
			if (t < 0.5f) {
				return 0.5f * Apply(2.0f * t, EaseType::EaseOutBounce);
			} else {
				return 1.0f - 0.5f * Apply(2.0f - 2.0f * t, EaseType::EaseOutBounce);
			}

		default:
			return t;
		}
	}
}