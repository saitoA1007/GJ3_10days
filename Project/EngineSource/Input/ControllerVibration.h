#pragma once

namespace GameEngine
{
	class Input;

	/// <summary>
	/// コントローラーの振動出力を管理する。
	/// </summary>
	class ControllerVibration final
	{
	public:
		explicit ControllerVibration(Input* input);
		~ControllerVibration();

		/// <summary>
		/// 左右のモーター強度を設定する。
		/// </summary>
		/// <param name="leftMotorSpeed">左モーター（低周波）の強度。0.0～1.0</param>
		/// <param name="rightMotorSpeed">右モーター（高周波）の強度。0.0～1.0</param>
		void SetVibration(float leftMotorSpeed, float rightMotorSpeed);

		/// <summary>
		/// 振動を停止する。
		/// </summary>
		void Stop();

	private:
		Input* input_ = nullptr;
		float leftMotorSpeed_ = 0.0f;
		float rightMotorSpeed_ = 0.0f;
	};
}
