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
