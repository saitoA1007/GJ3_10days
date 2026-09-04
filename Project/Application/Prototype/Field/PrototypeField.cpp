#include "PrototypeField.h"

#include <cassert>

using namespace GameEngine;

namespace {
	constexpr std::array<const char*, Prototype::kFieldZoneCount> kZoneNames = {
		"Center",
		"Near",
		"NearBuffer",
		"Middle",
		"MiddleBuffer",
		"Far",
		"OuterBuffer",
	};
}

namespace Prototype {

	Field::Field(Model* circleModel, const FieldSettings& settings)
		: settings_(settings) {
		assert(circleModel != nullptr && "Prototype field requires fieldCircle.obj");
		// 領域判定は配列の先頭から行うため、半径は必ず内側から昇順にする。
		for (size_t i = 0; i < settings_.radii.size(); ++i) {
			assert(settings_.radii[i] > 0.0f && "Field radii must be positive");
			assert((i == 0 || settings_.radii[i - 1] < settings_.radii[i]) &&
				"Field radii must increase from Center to OuterBuffer");
		}

		// 同一モデルを7枚重ね、色と半径だけを個別設定する。
		for (auto& zoneModel : zoneModels_) {
			zoneModel = std::make_unique<ModelComponent>(circleModel);
		}

		debugParameter_ = std::make_unique<DebugParameter>("PrototypeField");
		for (size_t i = 0; i < kFieldZoneCount; ++i) {
			debugParameter_->Register(kZoneNames[i], settings_.radii[i], static_cast<int>(i), "Radius");
			debugParameter_->Register(kZoneNames[i], settings_.colors[i], static_cast<int>(i), "Color");
		}
		debugParameter_->Apply();
	}

	void Field::Initialize() {
		ApplySettings();
	}

	void Field::Update() {
		if (debugParameter_->ApplyIfDirty()) {
			ApplySettings();
		}
	}

	void Field::DebugUpdate() {
		Update();
	}

	void Field::Draw() {
		// 大きな円から描画し、小さな円を上に重ねて各領域を見せる。
		for (size_t i = kFieldZoneCount; i-- > 0;) {
			zoneModels_[i]->DrawRaytracing(renderQueue_);
		}
	}

	FieldZone Field::GetZone(const Vector3& worldPosition) const {
		const float offsetX = worldPosition.x - settings_.center.x;
		const float offsetZ = worldPosition.z - settings_.center.z;
		const float distanceSquared = offsetX * offsetX + offsetZ * offsetZ;

		// 最初に収まった円が、その座標の最も内側の所属領域となる。
		for (size_t i = 0; i < settings_.radii.size(); ++i) {
			const float radius = settings_.radii[i];
			if (distanceSquared <= radius * radius) {
				return static_cast<FieldZone>(i);
			}
		}

		return FieldZone::Outside;
	}

	bool Field::Contains(const Vector3& worldPosition) const {
		return GetZone(worldPosition) != FieldZone::Outside;
	}

	float Field::GetRadius(FieldZone zone) const {
		const size_t index = static_cast<size_t>(zone);
		if (index >= settings_.radii.size()) {
			return 0.0f;
		}

		return settings_.radii[index];
	}

	void Field::ApplySettings() {
		for (size_t i = 0; i < zoneModels_.size(); ++i) {
			auto& zoneModel = zoneModels_[i];
			const float radius = settings_.radii[i];
			// 内側ほどわずかに高くし、重なった面のちらつきを防ぐ。
			const float height = static_cast<float>(kFieldZoneCount - 1 - i) * settings_.layerHeight;

			zoneModel->worldTransform_.transform_.scale = { radius, 1.0f, radius };
			zoneModel->worldTransform_.transform_.translate = {
				settings_.center.x,
				settings_.center.y + height,
				settings_.center.z,
			};

			zoneModel->materialData_->color = settings_.colors[i];
			zoneModel->materialData_->enableLighting = false;
			zoneModel->materialData_->metallic = 0.0f;
			zoneModel->materialData_->roughness = 1.0f;
			zoneModel->Update();
		}
	}
}
