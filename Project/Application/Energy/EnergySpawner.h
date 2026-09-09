#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "DebugParameter.h"
#include "IGameObject.h"

#include "Application/Field/Field.h"
#include "EnergyPickup.h"

/// エネルギー全体の生成設定。
struct EnergySpawnSettings 
{
	float spawnInterval = 2.5f;    // ランダム生成の間隔（秒）
	bool randomSpawnEnabled = true; // Playing中のランダムな自然生成を行うか
	float appearDuration = 1.0f;
	float groundHeight = 0.25f;    // 着地時のY座標
	float spawnAngleCenterDegrees = 0.0f; // 生成範囲の中心角（度）。0度は+X方向
	float spawnAngleRangeDegrees = 360.0f; // 中心角から左右へ広がる生成角度幅（度）
	float floatingAmplitude = 0.25f; // 着地後に上下する振幅
	float floatingSpeed = 2.0f;    // 浮遊アニメーションの角速度（rad/s）
	float rotationSpeed = 1.5f;    // 着地後のY軸回転速度（rad/s）
	int32_t maxActiveCount = 30;   // 落下・地上・運搬を含む同時存在上限
	int32_t initialCountPerZone = 0; // 開始時にNear/Middle/Farへ置く個数
	float lifetime = 10.0f;       
	float dissolveDuration = 1.0f;
};

/// @brief GamePlaying開始からの経過時間で発火する固定位置生成イベント。
struct ScheduledEnergySpawnEvent
{
	float timeSeconds = 0.0f; // GamePlaying開始から生成までの秒数
	int32_t energySize = static_cast<int32_t>(EnergySize::Small); // EnergySizeをEditorで扱うための整数値
	float arcPosition = 0.5f; // 生成角度範囲の左端0.0、中央0.5、右端1.0
	bool hasSpawned = false; // 現在のPlayingで生成済みか（保存対象外）
};

/// @brief 各フィールド領域の空からエネルギーを定期生成する。
class EnergySpawner final : public GameEngine::IGameObject
{
public:
	/// @brief 固定数のEnergyPickupを確保し、生成時に再利用する。
	/// @param[in] energyModel エネルギーの描画モデル。
	/// @param[in] field 生成領域の中心と半径を提供するフィールド。
	/// @param[in] capacity プールへ確保する最大個体数。
	EnergySpawner(
		GameEngine::Model* energyModel,
		Field* field,
		GameEngine::TextureManager* textureManager,
		GameEngine::Model* planeModel,
		size_t capacity = 64);
	~EnergySpawner() override = default;

	/// @brief 全個体をリセットし、Near・Middle・Farへ初期配置する。
	void Initialize() override;

	/// @brief 使用中の個体と定期生成タイマーを更新する。
	void Update() override;

	/// @brief 停止中の見た目とデバッグUIを更新する。
	void DebugUpdate() override;

	/// @brief プール内のアクティブなエネルギーを描画する。
	void Draw() override;

	/// @brief 全Energyと生成タイマーを初期状態へ戻し、設定された初期個数を再配置する。
	void ResetAll();

	/// @brief 落下と自動生成の有効状態を切り替える。
	/// @param[in] enabled 有効にするならtrue。
	void SetGameplayEnabled(bool enabled) { gameplayEnabled_ = enabled; }

	/// @brief ゲームプレイ処理が有効か取得する。
	/// @return 有効ならtrue。
	bool IsGameplayEnabled() const { return gameplayEnabled_; }

	/// @brief 定期的な自動生成の有効状態を切り替える。
	/// @param[in] enabled 有効にするならtrue。
	void SetAutoSpawnEnabled(bool enabled) { autoSpawnEnabled_ = enabled; }

	/// @brief 定期的な自動生成が有効か取得する。
	/// @return 有効ならtrue。
	bool IsAutoSpawnEnabled() const { return autoSpawnEnabled_; }

	/// @brief GamePlaying用の固定生成タイムラインを先頭から開始する。
	void BeginPlayingTimeline();

	/// @brief GamePlaying用の固定生成タイムラインを停止する。
	void EndPlayingTimeline();

	/// @brief 指定された生成可能領域へ空中生成する。
	/// @param[in] zone Near・Middle・Farのいずれか。
	/// @return 生成できた場合はtrue。
	bool SpawnInZone(FieldZone zone);

	/// @brief サイズ別領域の半円上にある指定位置へ空中生成する。
	/// @param[in] size 生成するエネルギーサイズ。
	/// @param[in] arcPosition 半円の左端を0、中央を0.5、右端を1とする位置。
	/// @return 生成できた場合はtrue。
	bool SpawnAtArcPosition(EnergySize size, float arcPosition);

	/// @brief 指定位置へ落下演出なしで直接生成する。
	/// @param[in] size 生成するエネルギーサイズ。
	/// @param[in] position 配置するワールド座標。
	/// @return 生成した個体。プールが満杯ならnullptr。
	EnergyPickup* SpawnOnGround(EnergySize size, const Vector3& position);

	/// @brief 範囲内にある未予約の地上エネルギーから最も近いものを探す。
	/// @param[in] position 検索中心のワールド座標。
	/// @param[in] maxDistance 検索する最大距離。
	/// @return 最も近い地上エネルギー。存在しなければnullptr。
	EnergyPickup* FindNearestAvailable(const Vector3& position, float maxDistance);

	/// @brief 現在アクティブなエネルギー数を取得する。
	/// @return アクティブ数。
	size_t GetActiveCount() const;

	/// @brief プールの最大個体数を取得する。
	/// @return 最大個体数。
	size_t GetCapacity() const { return pickups_.size(); }

	/// @brief 現在の生成設定を取得する。
	/// @return 生成設定への参照。
	const EnergySpawnSettings& GetSettings() const { return settings_; }

	const std::vector<EnergyPickup*>& GetEnergies() const { return activeEnergies_; }

private:
	/// @brief Register値を反映して安全な範囲へ補正する。
	void ApplyDebugParameters();

	/// @brief 個数・速度・獲得量などの不正値を補正する。
	void SanitizeSettings();

	/// @brief 使用中の全エネルギーを更新する。
	/// @param[in] deltaTime 前フレームからの経過秒数。
	void UpdatePickups(float deltaTime);

	/// @brief Near・Middle・Farから1領域を抽選して生成する。
	void SpawnRandom();

	/// @brief Playing開始からの経過時間に達した固定生成イベントを処理する。
	/// @param[in] deltaTime Playingの進行に使う経過秒数。
	void UpdatePlayingTimeline(float deltaTime);

	/// @brief 指定領域の円環と生成角度範囲内で面積一様なランダム位置を作る。
	/// @param[in] zone 生成対象のフィールド領域。
	/// @return 生成するワールド座標。
	Vector3 MakeSpawnPosition(FieldZone zone) const;

	/// @brief サイズ別領域の中央半径と半円上の指定位置から固定座標を作る。
	Vector3 MakeArcSpawnPosition(EnergySize size, float arcPosition) const;

	/// @brief 生成領域をSmall・Medium・Largeへ変換する。
	/// @param[in] zone 生成対象のフィールド領域。
	/// @return 領域に対応するエネルギーサイズ。
	EnergySize GetEnergySize(FieldZone zone) const;

	/// @brief エネルギーサイズを対応する生成領域へ変換する。
	FieldZone GetSpawnZone(EnergySize size) const;

	/// @brief JSONから読んだイベント数に合わせてEditorバインドを構築する。
	void InitializeTimelineEvents();

	/// @brief イベント数を末尾方向へ増減し、Editorバインドも同期する。
	void ResizeTimelineEvents(size_t count);

	/// @brief 1イベントをParameter Inspectorへ登録する。
	void RegisterTimelineEvent(size_t index);

	/// @brief EventCountのEditor値を現在値で登録し直す。
	void RegisterTimelineEventCount();

	/// @brief 任意位置のイベントを削除し、後続イベントを連番へ詰める。
	void RemoveTimelineEvent(size_t index);

	/// @brief タイムライン設定を有効範囲へ補正する。
	void SanitizeTimelineEvents();

	/// @brief 稼働数と手動生成ボタンをImGuiへ表示する。
	void DrawDebugWindow();

	Field* field_ = nullptr;                                       // 生成円環の中心と半径の参照先
	std::vector<std::unique_ptr<EnergyPickup>> pickups_;           // 再利用するエネルギーのプール
	std::vector<EnergyPickup*> activeEnergies_;
	EnergySpawnSettings settings_;                                 // 生成全体の設定
	// Small・Medium・Largeの順で保持する見た目と獲得量。
	std::array<EnergyTypeSettings, kEnergySizeCount> typeSettings_ = {
		EnergyTypeSettings{ 0.45f, 10, { 1.00f, 0.88f, 0.20f, 1.0f } },
		EnergyTypeSettings{ 0.70f, 25, { 0.25f, 0.85f, 1.00f, 1.0f } },
		EnergyTypeSettings{ 1.00f, 50, { 0.92f, 0.35f, 1.00f, 1.0f } },
		EnergyTypeSettings{ 2.00f, 0,  { 0.40f, 0.40f, 0.40f, 1.0f } },
	};
	std::unique_ptr<GameEngine::DebugParameter> debugParameter_;   // 設定とParameter Inspectorの接続
	float spawnTimer_ = 0.0f;                                      // 次回自動生成までに経過した秒数
	bool gameplayEnabled_ = true;                                  // Ready・TimeUp・Pause中はfalse
	bool autoSpawnEnabled_ = false;                                // Playing中だけtrue
	std::vector<std::unique_ptr<ScheduledEnergySpawnEvent>> timelineEvents_; // Editorで追加・削除する固定生成列
	int32_t timelineEventCount_ = 0;                               // JSONへ可変長イベント数を保存する値
	float playingTimelineElapsed_ = 0.0f;                          // GamePlaying開始からの経過秒数
	bool playingTimelineActive_ = false;                           // Playingフェーズ中だけtrue
};

