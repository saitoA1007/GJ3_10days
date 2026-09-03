#pragma once
#include <LogManager.h>
#include <Vector2.h>

//個人的にちょっとほしいなって思った関数群
namespace SF {
	void info(const std::string& message, const std::string& category);
	void warn(const std::string& message, const std::string& category);
	void error(const std::string& message, const std::string& category);

	Vector2 RotDir(const Vector2& origin, const Vector2& dir);

	//指定したディレクトリ内から、特定の拡張子と一致するファイルを探索する。ディレクトリは除外
	std::vector<std::string> SearchFiles(const std::filesystem::path& directory, const std::string& extension);
};
