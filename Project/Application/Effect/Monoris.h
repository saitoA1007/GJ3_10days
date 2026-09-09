#pragma once
#include <Model.h>
#include <WorldTransform.h>
#include <ModelComponent.h>
#include <IGameObject.h>

class Monoris : public GameEngine::IGameObject {
public:

	Monoris(GameEngine::Model* model);

	void SetMoveSpeed(float speed) { moveSpeed_ = speed; }

	void Initialize() override;
	void Update() override;
	void Draw() override;

	void DebugUpdate() override;

	static void SetTransform(const Transform& transform) { baseTransform_ = transform; }
	static void SetRadius(float radius) { radius_ = radius; }
	static void SetFloatingWidth(float width) { floatingWidth_ = width; }
	static void SetFloatingSpeed(float speed) { floatingSpeed_ = speed; }
	static void SetMoveSpeedRatio(float ratio) { moveSpeedRatio_ = ratio; }
	static void SetColor(Vector4 color) { color_ = color; }

private:

	float timer_ = 0.0f;
	float moveSpeed_ = 1.0f;
	float rotateSpeed_ = 1.0f;
	float radiusRatio_ = 0.0f;

	std::unique_ptr<GameEngine::ModelComponent> modelComponent_;
	static inline Vector4 color_ = { 1.0f,1.0f,1.0f,1.0f };

	static inline Transform baseTransform_;
	static inline float radius_ = 0.0f;

	static inline float moveSpeedRatio_ = 1.0f;

	static inline float floatingWidth_ = 2.0f;
	static inline float floatingSpeed_ = 2.0f;
};
