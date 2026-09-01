#pragma once
#include <LogManager.h>

//個人的にちょっとほしいなって思った関数群
namespace SF {
	void info(const std::string& message, const std::string& category);
	void warn(const std::string& message, const std::string& category);
	void error(const std::string& message, const std::string& category);
};

