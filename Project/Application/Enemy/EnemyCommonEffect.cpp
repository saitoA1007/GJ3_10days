#include "EnemyCommonEffect.h"

EnemyCommonEffect::EnemyCommonEffect(GameEngine::TextureManager* textureManager, GameEngine::ModelManager* modelManager) {
	constexpr uint32_t kMaxEffectNum = 128;
	effects_.resize(kMaxEffectNum);
	freeList_.resize(kMaxEffectNum);
	std::iota(freeList_.begin(), freeList_.end(), 0);
	freeListIndex_ = kMaxEffectNum - 1;
	
	auto planeModel = modelManager->GetNameByModel("plane.obj");

	for (uint32_t i = 0; i < kMaxEffectNum; ++i) {
		effects_[i].backCircle = std::make_unique<GameEngine::ParticleBehavior>("EnemyCommonEffect", 8, textureManager, planeModel);
	}
}

void EnemyCommonEffect::Initialize() {
	std::iota(freeList_.begin(), freeList_.end(), 0);
	freeListIndex_ = (uint32_t)freeList_.size() - 1;

	for (auto& effect : effects_) {
		effect.backCircle->Initialize();
	}
}

void EnemyCommonEffect::Update() {
	for (auto& effect : effects_) {
		effect.backCircle->Update();
	}
}

void EnemyCommonEffect::Draw() {
	for (auto& effect : effects_) {
		effect.backCircle->Draw();
	}
}

uint32_t EnemyCommonEffect::SecureEffectID() {
	if (freeListIndex_ < 0) {
		return UINT32_MAX; // エフェクトが足りない場合はUINT32_MAXを返す
	}
	uint32_t effectID = freeList_[freeListIndex_];
	freeListIndex_--;
	return effectID;
}

void EnemyCommonEffect::ReleaseEffectID(uint32_t effectID) {
	if (freeListIndex_ + 1 >= freeList_.size()) {
		return; // 解放できるエフェクトがない場合は何もしない
	}
	freeListIndex_++;
	freeList_[freeListIndex_] = effectID;
	effects_[effectID].backCircle->SetActive(false); // エフェクトを非アクティブ化
}

void EnemyCommonEffect::SetPosition(uint32_t effectID, const Matrix4x4& mat) {
	if (effectID >= effects_.size()) {
		return; // 無効なエフェクトIDの場合は何もしない
	}

	//effects_[effectID].backCircle->SetParent(mat);

	//仮置き
	effects_[effectID].backCircle->SetEmitterPos({ mat.m[3][0], mat.m[3][1], mat.m[3][2] });
}
