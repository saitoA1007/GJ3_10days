#pragma once
#include <IGameObject.h>
#include <ModelManager.h>
#include <AnimationManager.h>
#include <Animator.h>
#include <DebugParameter.h>
#include <WorldTransform.h>

class ResultMessage : public GameEngine::IGameObject {
public:

	enum Type {
		Mousukosi,
		Tyakuriku,
		Tobisugi,
		Count
	};

	ResultMessage(GameEngine::ModelManager* modelManager, GameEngine::AnimationManager* animationManager);

	void Initialize() override;
	void Update() override;
	void Draw() override;

	void DebugUpdate() override;

	//Typeに沿ったAnimationを起動して描画する。
	void Boot(Type type);

	bool IsFin() const;

private:

	struct ResultData {
		std::vector<GameEngine::Model*> models;
		AnimationData charAnimation;
	};

	std::vector<ResultData> resultDataList_;
	bool isBoot_ = false;
	Type currentType_ = Type::Mousukosi;

	//座標関係
	Transform transform_;
	float translateMergin_ = 0.0f;

	//Animation関係
	float timer_ = 0.0f;
	float diray_ = 0.1f;
	float floatingSpeed_ = 2.0f;

	GameEngine::DebugParameter debugParameter_{ "ResultMessage" };


	std::vector<std::unique_ptr<GameEngine::WorldTransform>> worldTransforms_;
	std::vector<GameEngine::Animator> animators_;
	
};
