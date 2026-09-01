#include "ShigeFunc.h"

void SF::info(const std::string& message, const std::string& category) {
	Log("[INFO]:" + message, category);
}

void  SF::warn(const std::string& message, const std::string& category) {
	Log("[WARN]:" + message, category);
}

void  SF::error(const std::string& message, const std::string& category) {
	Log("[ERROR]:" + message, category);
}

Vector2 SF::RotDir(const Vector2& origin, const Vector2& dir) {
	Vector2 tmp = dir;
	tmp.Normalize();

	return {
		origin.x * tmp.x - origin.y * tmp.y,
		origin.x * tmp.y + origin.y * tmp.x
	};
}
