#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "DebugParameter.h"
#include "IGameObject.h"

#include "Application/Prototype/Field/PrototypeField.h"
#include "PrototypeEnergyPickup.h"

namespace Prototype {

	/// @brief 空から降るエネルギー全体の生成設定。
	struct EnergySpawnSettings {
		float spawnInterval = 2.5f;    // ランダム生成の間隔（秒）
		float fallHeight = 10.0f;      // 地面からの生成高度
		float fallSpeed = 5.0f;        // 1秒あたりの落下距離
		float groundHeight = 0.25f;    // 着地時のY座標
		int32_t maxActiveCount = 30;   // 落下・地上・運搬を含む同時存在上限
		int32_t initialCountPerZone = 1; // 開始時にNear/Middle/Farへ置く個数
	};

	/// @brief 各フィールド領域の空からエネルギーを定期生成する。
	class EnergySpawner final : public GameEngine::IGameObject {
	public:
		/// @brief 固定数のEnergyPickupを確保し、生成時に再利用する。
		/// @param[in] energyModel エネルギーの描画モデル。
		/// @param[in] field 生成領域の中心と半径を提供するフィールド。
		/// @param[in] capacity プールへ確保する最大個体数。
		EnergySpawner(
			GameEngine::Model* energyModel,
			Field* field,
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

		/// @brief 落下と自動生成の有効状態を切り替える。
		/// @param[in] enabled 有効にするならtrue。
		void SetGameplayEnabled(bool enabled) { gameplayEnabled_ = enabled; }

		/// @brief ゲームプレイ処理が有効か取得する。
		/// @return 有効ならtrue。
		bool IsGameplayEnabled() const { return gameplayEnabled_; }

		/// @brief 指定された生成可能領域へ空中生成する。
		/// @param[in] zone Near・Middle・Farのいずれか。
		/// @return 生成できた場合はtrue。
		bool SpawnInZone(FieldZone zone);

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

		/// @brief 指定領域の円環内で面積一様なランダム位置を作る。
		/// @param[in] zone 生成対象のフィールド領域。
		/// @return 生成するワールド座標。
		Vector3 MakeSpawnPosition(FieldZone zone) const;

		/// @brief 生成領域をSmall・Medium・Largeへ変換する。
		/// @param[in] zone 生成対象のフィールド領域。
		/// @return 領域に対応するエネルギーサイズ。
		EnergySize GetEnergySize(FieldZone zone) const;

		/// @brief 稼働数と手動生成ボタンをImGuiへ表示する。
		void DrawDebugWindow();

		Field* field_ = nullptr;                                       // 生成円環の中心と半径の参照先
		std::vector<std::unique_ptr<EnergyPickup>> pickups_;           // 再利用するエネルギーのプール
		EnergySpawnSettings settings_;                                 // 生成全体の設定
		// Small・Medium・Largeの順で保持する見た目と獲得量。
		std::array<EnergyTypeSettings, kEnergySizeCount> typeSettings_ = {
			EnergyTypeSettings{ 0.45f, 10, { 1.00f, 0.88f, 0.20f, 1.0f } },
			EnergyTypeSettings{ 0.70f, 25, { 0.25f, 0.85f, 1.00f, 1.0f } },
			EnergyTypeSettings{ 1.00f, 50, { 0.92f, 0.35f, 1.00f, 1.0f } },
		};
		std::unique_ptr<GameEngine::DebugParameter> debugParameter_;   // 設定とParameter Inspectorの接続
		float spawnTimer_ = 0.0f;                                      // 次回自動生成までに経過した秒数
		bool gameplayEnabled_ = true;                                  // Ready・TimeUp・Pause中はfalse
	};
}
