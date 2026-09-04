#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "ModelComponent.h"
#include "Vector3.h"
#include "Vector4.h"

namespace GameEngine {
	class Model;
	class RenderQueue;
}

namespace Prototype {

	enum class EnergySize : uint8_t {
		Small,
		Medium,
		Large,
		Count,
	};

	inline constexpr size_t kEnergySizeCount = static_cast<size_t>(EnergySize::Count);

	enum class EnergyState : uint8_t {
		Inactive,
		Falling,
		OnGround,
		Reserved,
		Carried,
	};

	struct EnergyTypeSettings {
		float scale = 1.0f;
		int32_t value = 10;
		Vector4 color = { 1.0f, 0.9f, 0.2f, 1.0f };
	};

	/// <summary>
	/// フィールドに落下し、ユニットによって運ばれるエネルギー
	/// </summary>
	class EnergyPickup final {
	public:
		explicit EnergyPickup(GameEngine::Model* model);

		void Spawn(
			EnergySize size,
			const Vector3& groundPosition,
			float fallHeight,
			float fallSpeed,
			const EnergyTypeSettings& typeSettings);
		void Reset();
		void Update(float deltaTime);
		void Draw(GameEngine::RenderQueue* renderQueue);

		bool TryReserve();
		bool BeginCarry();
		void SetCarriedPosition(const Vector3& position);
		void DropOnGround(const Vector3& position);
		int32_t Deliver();

		void ApplyTypeSettings(const EnergyTypeSettings& settings);
		void SetHighlighted(bool highlighted);

		bool IsActive() const { return state_ != EnergyState::Inactive; }
		bool IsTargetable() const { return state_ == EnergyState::OnGround; }
		bool IsReserved() const { return state_ == EnergyState::Reserved; }
		bool IsCarried() const { return state_ == EnergyState::Carried; }
		EnergyState GetState() const { return state_; }
		EnergySize GetSize() const { return size_; }
		int32_t GetValue() const { return typeSettings_.value; }
		float GetScale() const { return typeSettings_.scale; }
		const Vector3& GetPosition() const { return position_; }

	private:
		void SyncModel();

		std::unique_ptr<GameEngine::ModelComponent> modelComponent_;
		EnergySize size_ = EnergySize::Small;
		EnergyState state_ = EnergyState::Inactive;
		EnergyTypeSettings typeSettings_;
		Vector3 position_ = {};
		float groundY_ = 0.0f;
		float fallSpeed_ = 0.0f;
		bool isHighlighted_ = false;
	};
}
