#pragma once
#include <IGameObject.h>
#include <ModelManager.h>
#include <WorldTransform.h>

//GameObjectManagerに委託せず、他のクラスによる管理を行うことを推奨
//isActive_で描画の有効無効を切り替える
class ShuffleNumber : public GameEngine::IGameObject {
public:

	ShuffleNumber(GameEngine::ModelManager* modelManager);
	~ShuffleNumber();

	void Initialize() override;
	void Update() override;
	void Draw() override;

	void DebugUpdate() override;

	void SetTransform(const Transform& transform);

	// シャッフル開始
	void ShuffleStart();
	// シャッフル停止(初期値だと現在の値で止まる)
	void Stop(int number = -1);

private:

	std::vector<GameEngine::Model*> numberModels_;

	GameEngine::WorldTransform worldTransform_;

	int currentIndex_ = 0;

	bool shuffling_ = false;
};
