#include "GamingColor.h"

#include <algorithm>
#include <cmath>

#include "ModelComponent.h"
#include "MyMath.h"

using namespace GameEngine;

void GamingColor::Update(float deltaTime) {
	const float cycleSpeed = (std::max)(settings_.cycleSpeed, 0.0f);
	hue_ = NormalizeHue(hue_ + (std::max)(deltaTime, 0.0f) * cycleSpeed);
}

void GamingColor::Reset(float hue) {
	hue_ = NormalizeHue(hue);
}

void GamingColor::Apply(ModelComponent& model, std::size_t colorIndex) const {
	model.SetColor(GetColor(colorIndex));
	model.SetEnableLighting(!settings_.selfIlluminated);
}

Vector3 GamingColor::GetColor(std::size_t colorIndex) const {
	const float hue = NormalizeHue(
		hue_ + settings_.colorSpacing * static_cast<float>(colorIndex)
	);
	const float saturation = std::clamp(settings_.saturation, 0.0f, 1.0f);
	const float brightness = std::clamp(settings_.brightness, 0.0f, 1.0f);
	return Math::HSVtoRGB(hue, saturation, brightness);
}

float GamingColor::NormalizeHue(float hue) {
	hue = std::fmod(hue, 1.0f);
	return hue < 0.0f ? hue + 1.0f : hue;
}
