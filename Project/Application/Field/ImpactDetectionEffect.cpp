#include "ImpactDetectionEffect.h"
#include "MyMath.h"
#include "FPSCounter.h"
#include "ImGuiManager.h"
#include <algorithm>
#include <cmath>
using namespace GameEngine;

ImpactDetectionEffect::ImpactDetectionEffect(GameEngine::Model* model, uint32_t texture) {
	model_ = model;
	textureGH_ = texture;

	// メモリ確保
	particles_.resize(maxNum_);
	impacts_.reserve(maxImpactNum_);

	// 格子状に並べる
	ResetGrid();

	// パラメータの登録
	debugParame_ = std::make_unique<GameEngine::DebugParameter>("ImpactDetectionEffect");
	debugParame_->Register("Spacing", spacing_, 0, "Grid");
	debugParame_->Register("Center", gridCenter_, 1, "Grid");
	debugParame_->Register("Speed", waveSpeed_, 0, "Wave");
	debugParame_->Register("Width", waveWidth_, 1, "Wave");
	debugParame_->Register("Count", waveCount_, 2, "Wave");
	debugParame_->Register("Height", waveHeight_, 3, "Wave");
	debugParame_->Register("FadeDistance", fadeDistance_, 4, "Wave");
	debugParame_->Register("ColorHeight", colorHeight_, 0, "Look");
	debugParame_->Register("BaseScale", baseScale_, 1, "Look");
	debugParame_->Register("ScalePunch", scalePunch_, 2, "Look");
	debugParame_->Register("IdleColor", idleColor_, 3, "Look");
	debugParame_->Register("CrestColor", crestColor_, 4, "Look");
	debugParame_->Register("TroughColor", troughColor_, 5, "Look");
	debugParame_->Apply();

	// 初期化
	worldTransforms_.Initialize(maxNum_, {});
	Update();
}

void ImpactDetectionEffect::Initialize() {

}

void ImpactDetectionEffect::Update() {

	// 調整した値を反映する
	if (debugParame_->ApplyIfDirty()) {
		ResetGrid();
	}

	float deltaTime = FpsCounter::gameDeltaTime;

	// 衝撃の時間を進める
	for (ImpactSource& impact : impacts_) {
		impact.elapsedTime += deltaTime;
	}
	// 波面が減衰しきる距離を通り過ぎた衝撃を消す
	std::erase_if(impacts_, [this](const ImpactSource& impact) {
		return impact.elapsedTime * waveSpeed_ - waveWidth_ > fadeDistance_;
		});

	// カメラを取得
	Matrix4x4 cameraMatrix = renderQueue_->GetMainCamera().GetWorldMatrix();
	cameraMatrix = renderQueue_->GetDebugCameraWorldMatrix();

	// 更新
    for (uint32_t i = 0; i < maxNum_; ++i) {
        ParticleData& particle = particles_[i];

		// 発生している衝撃をすべて足し合わせて高さを求める
		float height = 0.0f;
		for (const ImpactSource& impact : impacts_) {
			height += EvaluateWave(impact, particle.basePos);
		}

		// y軸の動きで衝撃の伝わり方を表現する
		particle.transform.translate = particle.basePos;
		particle.transform.translate.y += height;

		// 揺れの大きさを0~1にして、色と大きさに使う
		float intensity = std::clamp(std::fabs(height) / colorHeight_, 0.0f, 1.0f);

		// 持ち上がっている所は山の色、沈んでいる所は谷の色にする
		const Vector4& waveColor = (height >= 0.0f) ? crestColor_ : troughColor_;
		particle.color = idleColor_ + waveColor * intensity;

		// 波が通っている所だけ大きくする
		float scale = baseScale_ * (1.0f + intensity * scalePunch_);
		particle.transform.scale = { scale,scale,scale };

		worldTransforms_.transformDatas_[i].textureHandle = textureGH_;
		worldTransforms_.transformDatas_[i].color = particle.color;
        worldTransforms_.transformDatas_[i].worldMatrix = Math::MakeBillboardMatrix(particle.transform.scale, particle.transform.translate, particle.transform.rotate.z, cameraMatrix);
    }
}

void ImpactDetectionEffect::DebugUpdate() {
#ifdef USE_IMGUI
	ImGui::Begin("ImpactDetectionEffect");

	static Vector3 debugPos = { 0.0f,0.0f,0.0f };
	static float debugPower = 1.0f;
	ImGui::DragFloat3("ImpactPos", &debugPos.x, 0.1f);
	ImGui::DragFloat("ImpactPower", &debugPower, 0.1f);
	if (ImGui::Button("ApplyImpact")) {
		ApplayImpact(debugPos, debugPower);
	}
	ImGui::Text("ActiveImpact : %d", static_cast<int>(impacts_.size()));

	ImGui::End();
#endif
}

void ImpactDetectionEffect::Draw() {
	// 描画
	renderQueue_->SubmitInstancing(model_, maxNum_, worldTransforms_, 0.0f, BlendMode::kBlendModeAdd, nullptr, "WBOITAccumulatePass");
}

void ImpactDetectionEffect::ResetGrid() {

	// 中心を原点に揃えるためのオフセット
	float offset = (static_cast<float>(row_) - 1.0f) * spacing_ * 0.5f;

	uint32_t num = 0;
	for (uint32_t z = 0; z < row_; ++z) {
		for (uint32_t x = 0; x < row_; ++x) {
			particles_[num].basePos = {
				gridCenter_.x + static_cast<float>(x) * spacing_ - offset,
				gridCenter_.y,
				gridCenter_.z + static_cast<float>(z) * spacing_ - offset };
			particles_[num].transform.translate = particles_[num].basePos;
			num++;
		}
	}
}

void ImpactDetectionEffect::ApplayImpact(Vector3 pos, float power) {

	// 数が多すぎたら一番古い衝撃を捨てる
	if (impacts_.size() >= maxImpactNum_) {
		impacts_.erase(impacts_.begin());
	}

	ImpactSource source{};
	source.pos = pos;
	source.power = power;
	source.elapsedTime = 0.0f;
	impacts_.push_back(source);
}

float ImpactDetectionEffect::EvaluateWave(const ImpactSource& source, const Vector3& particlePos) const {

	// xz平面上での発生源からの距離
	float dx = particlePos.x - source.pos.x;
	float dz = particlePos.z - source.pos.z;
	float distance = std::sqrt(dx * dx + dz * dz);

	// 波面が今どこまで進んでいるか
	float wavefront = source.elapsedTime * waveSpeed_;

	// 波面から見た位置。0未満はまだ波が届いておらず、厚みを超えたら通り過ぎている
	float phase = wavefront - distance;
	if (phase < 0.0f || phase > waveWidth_) {
		return 0.0f;
	}

	// 波の形。山と谷を、両端が0になる包絡線で包んで繋ぎ目を目立たなくする
	float t = phase / waveWidth_;
	float shape = std::sin(t * PI) * std::cos(t * TWO_PI * waveCount_);

	// 遠いほど弱まる
	float attenuation = 1.0f - (distance / fadeDistance_);
	if (attenuation <= 0.0f) {
		return 0.0f;
	}
	attenuation *= attenuation;

	return source.power * waveHeight_ * shape * attenuation;
}
