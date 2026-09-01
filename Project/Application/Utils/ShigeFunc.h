#pragma once
#include <LogManager.h>
#include <Vector2.h>

//個人的にちょっとほしいなって思った関数群
namespace SF {
	void info(const std::string& message, const std::string& category);
	void warn(const std::string& message, const std::string& category);
	void error(const std::string& message, const std::string& category);

	Vector2 RotDir(const Vector2& origin, const Vector2& dir);
};

