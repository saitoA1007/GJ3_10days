#include "ControllerVibration.h"

#include <algorithm>

#include "InPut.h"

using namespace GameEngine;

ControllerVibration::ControllerVibration(Input* input)
	: input_(input)
{
}

ControllerVibration::~ControllerVibration()
{
	Stop();
}

void ControllerVibration::SetVibration(float leftMotorSpeed, float rightMotorSpeed)
{
	leftMotorSpeed = std::clamp(leftMotorSpeed, 0.0f, 1.0f);
	rightMotorSpeed = std::clamp(rightMotorSpeed, 0.0f, 1.0f);

	// 同じ強度を毎フレームXInputへ送り直さない。
	if (leftMotorSpeed_ == leftMotorSpeed && rightMotorSpeed_ == rightMotorSpeed)
	{
		return;
	}

	leftMotorSpeed_ = leftMotorSpeed;
	rightMotorSpeed_ = rightMotorSpeed;

	if (input_)
	{
		input_->SetVibration(leftMotorSpeed_, rightMotorSpeed_);
	}
}

void ControllerVibration::Stop()
{
	SetVibration(0.0f, 0.0f);
}
