#include "MoonObject.h"
#include "Application/CollisionConfig.h"
using namespace GameEngine;

MoonObject::MoonObject(GameEngine::Model* model, GameEngine::Model* fractureModel, uint32_t moonGH, uint32_t norGH)
	: defaultModel_(model), destructObject_("MoonObject", fractureModel, static_cast<uint32_t>(CollisionTypeID::kWall), kCollisionAttributeTerrain) {

	defaultModel_.materialData_->textureHandle = moonGH;
	defaultModel_.materialData_->normalTextureHandle = norGH;

	destructObject_.SetMaterial(&defaultModel_.GetMaterial());

	std::string subGroup = "Collider";

	// 登録
	debugParam_.RegisterWorld("", defaultModel_.worldTransform_);
	debugParam_.Register("pPos", pPos_);
	debugParam_.Register("Size", colliderSize_, 0, subGroup);
	subGroup = "InputDestruct";
	debugParam_.Register("damageAmount", destructObject_.damageAmount_, 0, subGroup);
	debugParam_.Register("craterRadius", destructObject_.craterRadius_, 0, subGroup);
	subGroup = "Material";
	debugParam_.Register("Color", defaultModel_.materialData_->color, 0, subGroup);
	debugParam_.Register("Metalic", defaultModel_.materialData_->metallic, 0, subGroup);
	debugParam_.Register("Roughess", defaultModel_.materialData_->roughness, 0, subGroup);

	debugParam_.Apply();
	destructObject_.worldTransform_.transform_ = defaultModel_.worldTransform_.transform_;

	// 当たり判定
	collider_.SetWorldPosition(destructObject_.worldTransform_.transform_.translate);
	collider_.SetSize(colliderSize_);
	collider_.UpdateOrientationsFromRotate(destructObject_.worldTransform_.transform_.rotate);
	collider_.SetCollisionAttribute(kCollisionAttributeTerrain);
	collider_.SetCollisionMask(~kCollisionAttributeTerrain);
	// データを登録
	UserData userData;
	userData.typeID = static_cast<uint32_t>(CollisionTypeID::kWall);
	userData.object = this;
	collider_.SetUserData(userData);
	// コールバック関数に登録する
	collider_.SetOnCollisionCallback([this](const CollisionResult& result) {
		this->OnCollisionEnter(result);
	});

	// 破壊用当たり判定
	pCollider_.SetWorldPosition(pPos_);
	pCollider_.SetRadius(5.0f);
	pCollider_.SetCollisionAttribute(kCollisionAttributePlayer);
	pCollider_.SetCollisionMask(~kCollisionAttributePlayer);
}

void MoonObject::Initialize() {

}

void MoonObject::Update() {
	if (debugParam_.ApplyIfDirty()) {
		destructObject_.worldTransform_.transform_ = defaultModel_.worldTransform_.transform_;
		collider_.SetWorldPosition(destructObject_.worldTransform_.transform_.translate);
		collider_.SetSize(colliderSize_);
	}

	defaultModel_.Update();
	// 破片の更新処理
	destructObject_.Update();
	// 位置を更新
	pCollider_.SetWorldPosition(pPos_);
}

void MoonObject::Draw() {

	if (isFractureActive_) {
		// 破片を描画
		destructObject_.NewDraw(&defaultModel_.GetMaterial());
	} else {
		defaultModel_.DrawRaytracing(renderQueue_);
	}
}

void MoonObject::OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result) {
	if (!isFractureActive_) { return; }
	// 破片を飛び散らせる
	destructObject_.OnCollisionEnter(result);
}

void MoonObject::Reset() {
	pPos_.x = 60.0f;

	// カケラを元に戻す
	if (isFractureActive_) {
		destructObject_.Reassemble();
	}

	isFractureActive_ = false;
}