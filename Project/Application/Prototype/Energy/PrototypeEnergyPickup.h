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

	/// @brief エネルギーの大きさ。Scaleと獲得量は設定側で決まる。
	enum class EnergySize : uint8_t {
		Small,
		Medium,
		Large,
		Count,
	};

	inline constexpr size_t kEnergySizeCount = static_cast<size_t>(EnergySize::Count);

	/// @brief エネルギー1個のライフサイクル。
	enum class EnergyState : uint8_t {
		Inactive, // プール内で未使用
		Falling,  // 空中から落下中
		OnGround, // 地面にあり、選択可能
		Reserved, // 派遣ユニットが確保済み
		Carried,  // ユニットが運搬中
	};

	/// @brief サイズごとに異なる見た目と獲得量。
	struct EnergyTypeSettings {
		float scale = 1.0f;                              // モデルの一様スケール
		int32_t value = 10;                              // ロケットへ届けたときの増加量
		Vector4 color = { 1.0f, 0.9f, 0.2f, 1.0f };     // 通常時の表示色
	};

	/// @brief フィールドに落下し、ユニットによって運ばれるエネルギー。
	class EnergyPickup final {
	public:
		/// @brief プールで再利用するエネルギーモデルを準備する。
		/// @param[in] model エネルギーの描画モデル。
		explicit EnergyPickup(GameEngine::Model* model);

		/// @brief 空中から落下するエネルギーとして生成する。
		/// @param[in] size 生成するエネルギーサイズ。
		/// @param[in] groundPosition 着地するワールド座標。
		/// @param[in] fallHeight 地面からの生成高度。
		/// @param[in] fallSpeed 1秒あたりの落下距離。
		/// @param[in] typeSettings サイズに対応する見た目と獲得量。
		void Spawn(
			EnergySize size,
			const Vector3& groundPosition,
			float fallHeight,
			float fallSpeed,
			const EnergyTypeSettings& typeSettings);
		/// @brief 敵のドロップなど、落下演出なしで地面へ直接生成する。
		/// @param[in] size 生成するエネルギーサイズ。
		/// @param[in] groundPosition 配置する地面のワールド座標。
		/// @param[in] typeSettings サイズに対応する見た目と獲得量。
		void SpawnOnGround(
			EnergySize size,
			const Vector3& groundPosition,
			const EnergyTypeSettings& typeSettings);
		/// @brief 非アクティブ状態へ戻して再利用可能にする。
		void Reset();

		/// @brief 落下中の位置とモデルを更新する。
		/// @param[in] deltaTime 前フレームからの経過秒数。
		void Update(float deltaTime);

		/// @brief アクティブな場合だけ描画する。
		/// @param[in] renderQueue 描画命令の登録先。
		void Draw(GameEngine::RenderQueue* renderQueue);

		/// @brief OnGround状態ならユニット用に確保する。
		/// @return 予約できた場合はtrue。
		bool TryReserve();

		/// @brief Reserved状態からCarried状態へ移す。
		/// @return 運搬を開始できた場合はtrue。
		bool BeginCarry();

		/// @brief 運搬中のエネルギーをユニット頭上へ追従させる。
		/// @param[in] position 新しいワールド座標。
		void SetCarriedPosition(const Vector3& position);

		/// @brief 運搬失敗地点へ戻し、再選択可能にする。
		/// @param[in] position 落とされたワールド座標。
		void DropOnGround(const Vector3& position);

		/// @brief ロケットへの納品を確定して非アクティブにする。
		/// @return ロケットへ加算する獲得量。運搬中でなければ0。
		int32_t Deliver();

		/// @brief 実行中のRegister変更を現在の個体へ反映する。
		/// @param[in] settings サイズに対応する新しい設定。
		void ApplyTypeSettings(const EnergyTypeSettings& settings);

		/// @brief カーソル選択中の強調表示を切り替える。
		/// @param[in] highlighted 選択中として表示するならtrue。
		void SetHighlighted(bool highlighted);

		/// @brief プール内で使用中かを取得する。
		/// @return Inactive以外ならtrue。
		bool IsActive() const { return state_ != EnergyState::Inactive; }
		/// @brief 新しくロックオンできるかを取得する。
		/// @return OnGroundならtrue。
		bool IsTargetable() const { return state_ == EnergyState::OnGround; }
		/// @brief ユニットが予約済みかを取得する。
		/// @return Reservedならtrue。
		bool IsReserved() const { return state_ == EnergyState::Reserved; }
		/// @brief ユニットが運搬中かを取得する。
		/// @return Carriedならtrue。
		bool IsCarried() const { return state_ == EnergyState::Carried; }
		/// @brief 現在のライフサイクル状態を取得する。
		/// @return 現在状態。
		EnergyState GetState() const { return state_; }
		/// @brief 現在のエネルギーサイズを取得する。
		/// @return 現在サイズ。
		EnergySize GetSize() const { return size_; }
		/// @brief ロケットへ届けたときの獲得量を取得する。
		/// @return 獲得量。
		int32_t GetValue() const { return typeSettings_.value; }
		/// @brief 現在の表示スケールを取得する。
		/// @return 一様スケール値。
		float GetScale() const { return typeSettings_.scale; }
		/// @brief 現在のワールド座標を取得する。
		/// @return 現在位置への参照。
		const Vector3& GetPosition() const { return position_; }

	private:
		/// @brief 現在位置・サイズ・色をモデルへ反映する。
		void SyncModel();

		std::unique_ptr<GameEngine::ModelComponent> modelComponent_; // 数字ではなくenergy.objの描画情報
		EnergySize size_ = EnergySize::Small;                        // この個体の現在サイズ
		EnergyState state_ = EnergyState::Inactive;                  // 現在のライフサイクル状態
		EnergyTypeSettings typeSettings_;                            // サイズに対応する設定のコピー
		Vector3 position_ = {};                                      // 現在のワールド座標
		float groundY_ = 0.0f;                                      // 落下終了・再配置に使う地面の高さ
		float fallSpeed_ = 0.0f;                                    // 1秒あたりの落下距離
		bool isHighlighted_ = false;                                 // カーソル選択中か
	};
}
