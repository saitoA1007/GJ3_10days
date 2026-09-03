#include "ShigeFunc.h"

namespace fs = std::filesystem;

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

std::vector<std::string> SF::SearchFiles(const std::filesystem::path& directory, const std::string& extension) {
    std::vector<std::string> contents;

    if (!fs::exists(directory) || !fs::is_directory(directory)) {
        return {};
    }

    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == extension) {
            fs::path relativePath = entry.path().lexically_relative(directory);
            contents.push_back(relativePath.generic_string());
        }
    }

    return contents;
}
