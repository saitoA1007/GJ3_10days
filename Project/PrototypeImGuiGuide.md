# Prototype ImGui・Parameter Inspector 操作ガイド

## この資料について

`ProtoScene`で使用するImGuiデバッグウィンドウと、Register式のParameter Inspectorに登録されている値をまとめた資料です。

- 「現在値」は`Resources/Json/GameData/Prototype*.json`に保存されている値を基準にしています。
- `Vector3`は基本的に`X, Y, Z`、`Vector4`の色は`R, G, B, A`です。
- 時間の単位は秒、位置・距離・速度はゲーム内のワールド単位です。
- 多くの項目は変更後すぐ反映されます。初期化時だけ使われる項目は個別に記載しています。
- 不正な負数などは各システム内で補正される場合があります。

## 操作方法

| 操作 | マウス | キーボード | ゲームパッド |
|---|---|---|---|
| カーソル移動 | マウス移動 | `WASD` | 左スティック |
| 対象選択・チャージ | 左クリック | `Space` | Aボタン |
| 0消費で派遣 | 0.2秒以内の短押し | 0.2秒以内の短押し | 0.2秒以内の短押し |

カーソルの検索範囲内にEnergyとEnemyの両方が存在する場合は、カーソルに近い方が選ばれます。同距離の場合はEnemyが優先されます。

---

## PrototypeField

円形フィールド7枚の半径と色を調整します。専用のImGuiウィンドウはなく、Parameter Inspectorから操作します。

### Radius

| 項目 | 現在値 | 変更される内容 |
|---|---:|---|
| `Center` | 5.0 | ロケット周辺の中心領域の外周半径 |
| `Near` | 10.0 | Small Energy生成領域の外周半径 |
| `NearBuffer` | 15.0 | NearとMiddleの間にある生成禁止帯の外周半径 |
| `Middle` | 20.0 | Medium Energy生成領域の外周半径 |
| `MiddleBuffer` | 25.0 | MiddleとFarの間にある生成禁止帯の外周半径 |
| `Far` | 30.0 | Large Energy生成領域の外周半径 |
| `OuterBuffer` | 35.0 | 敵が出現するフィールド最外周の半径 |

半径は`Center < Near < NearBuffer < Middle < MiddleBuffer < Far < OuterBuffer`の順を維持してください。順番が崩れると、領域判定やEnergyの生成範囲が正しくなくなります。

### Color

| 項目 | 現在値 | 変更される内容 |
|---|---|---|
| `Center` | `(0.86, 0.92, 1.00, 1.00)` | Center円の色 |
| `Near` | `(0.28, 0.68, 0.48, 1.00)` | Near円の色 |
| `NearBuffer` | `(1.00, 1.00, 1.00, 1.00)` | NearBuffer円の色 |
| `Middle` | `(0.86, 0.62, 0.24, 1.00)` | Middle円の色 |
| `MiddleBuffer` | `(1.00, 1.00, 1.00, 1.00)` | MiddleBuffer円の色 |
| `Far` | `(0.68, 0.28, 0.34, 1.00)` | Far円の色 |
| `OuterBuffer` | `(1.00, 1.00, 1.00, 1.00)` | OuterBuffer円の色 |

半径と色は変更後すぐ、領域判定と表示モデルへ反映されます。

---

## PrototypeRocket

ロケットの位置、当たり判定、保有Energyを調整します。

### Parameter Inspector

| グループ | 項目 | 現在値 | 変更される内容 | 反映タイミング・注意点 |
|---|---|---:|---|---|
| `Transform` | `Position` | `(0.0, 0.25, 0.0)` | ロケットのワールド座標 | モデルとColliderへ即時反映 |
| `Transform` | `Scale` | `(1.0, 1.0, 1.0)` | ロケットモデルの表示倍率 | 0未満の各成分は0へ補正 |
| `Collider` | `Radius` | 2.5 | 敵の到達判定半径 | 0未満は0へ補正 |
| `Collider` | `OffsetY` | 0.0 | ロケット座標からCollider中心までのY差 | 即時反映 |
| `Energy` | `InitialEnergy` | 100 | ゲーム開始時・Reset時のEnergy | 変更だけでは現在値は変わらない |
| `Energy` | `EnemyHitLoss` | 10 | 敵1体が到達した際のEnergy減少量 | 次の敵到達から反映 |
| `Debug` | `ChangeAmount` | 10 | `Deposit`と`Consume`ボタンが増減する量 | 0未満は0へ補正 |

### ImGui: `Prototype Rocket`

| 表示・ボタン | 操作すると変更される値 |
|---|---|
| `Energy` | 現在のロケットEnergyを表示するだけで、操作はしない |
| `Deposit` | 現在Energyへ`ChangeAmount`を加算 |
| `Consume` | 現在Energyから`ChangeAmount`を減算。残量より多い場合は0で止まる |
| `Reset` | 現在Energyを`InitialEnergy`へ戻す |

---

## PrototypeEnergy

空から降るEnergyの生成条件と、Small・Medium・Largeそれぞれの見た目・獲得量を調整します。

### Spawn

| 項目 | 現在値 | 変更される内容 | 補正・注意点 |
|---|---:|---|---|
| `SpawnInterval` | 2.5 | ランダムな領域へEnergyを1個生成する間隔 | 最低0.1秒 |
| `FallHeight` | 10.0 | 地面からの生成高度 | 最低0。次に空中生成する個体から反映 |
| `FallSpeed` | 5.0 | 1秒あたりの落下距離 | 最低0.01。次に空中生成する個体から反映 |
| `GroundHeight` | 0.25 | 着地・敵ドロップ時のY座標 | 次に生成する個体から反映 |
| `MaxActiveCount` | 30 | 落下中・地上・予約・運搬中を含む同時存在上限 | 1～プール容量。値を下げても既存個体は削除しない |
| `InitialCountPerZone` | 1 | 開始時にNear・Middle・Farへ配置する個数 | 次回初期化時に反映。最大は`MaxActiveCount / 3` |

自動生成先はNear・Middle・Farからランダムに選ばれます。Centerと各Bufferには空からEnergyが降りません。

### Type

| サイズ | 生成領域 | `Scale` | `Value` | `Color` |
|---|---|---:|---:|---|
| `Small` | Near | 0.5 | 10 | `(1.00, 0.88, 0.20, 1.00)` |
| `Medium` | Middle | 0.7 | 25 | `(0.25, 0.85, 1.00, 1.00)` |
| `Large` | Far | 1.0 | 50 | `(0.92, 0.35, 1.00, 1.00)` |

- `Scale`はEnergyモデルの表示倍率です。0未満は0へ補正されます。
- `Value`はロケットへ届けた際の獲得量です。0未満は0へ補正されます。
- `Color`は通常時のRGBA色です。
- Typeの変更は、すでにフィールド上に存在する同サイズのEnergyにも反映されます。

### ImGui: `Prototype Energy`

| 表示・ボタン | 操作すると変更される値 |
|---|---|
| `Active` | 現在使用中のEnergy数とプール容量を表示 |
| `Spawn Near / Small` | Near領域の空中にSmallを1個生成 |
| `Spawn Middle / Medium` | Middle領域の空中にMediumを1個生成 |
| `Spawn Far / Large` | Far領域の空中にLargeを1個生成 |

ゲームプレイが無効なReady・TimeUp・Pause中、または`MaxActiveCount`到達時は、生成ボタンを押しても生成されません。

---

## PrototypeUnit

ユニット数、移動速度、スタミナ消費、色を調整します。

### Manager

| 項目 | 現在値 | 変更される内容 | 補正・注意点 |
|---|---:|---|---|
| `UnitCount` | 5 | ゲームへ参加するユニット数 | 1～プール容量。減らした範囲に出撃中ユニットがいる場合は回収される |

### Transform

| 項目 | 現在値 | 変更される内容 |
|---|---|---|
| `LaunchOffset` | `(0.0, 0.0, 0.0)` | ロケット位置から出撃位置までの差 |
| `Scale` | `(0.7, 0.7, 0.7)` | ユニットモデルの表示倍率 |
| `CarryOffset` | `(0.0, 1.45, 0.0)` | ユニット位置から頭上Energyまでの差 |

`Scale`の0未満の各成分は0へ補正されます。

### Move・Collision・Stamina

| グループ | 項目 | 現在値 | 変更される内容 | 補正・注意点 |
|---|---|---:|---|---|
| `Move` | `NormalSpeed` | 1.5 | スタミナ切れ時の移動速度 | 最低0 |
| `Move` | `BoostedSpeed` | 5.0 | スタミナがある間の移動速度 | `NormalSpeed`以上へ補正 |
| `Move` | `PickupRadius` | 0.45 | Energy回収が成立する距離 | 最低0 |
| `Move` | `DeliveryRadius` | 1.6 | ロケットへの納品が成立する距離 | 最低0 |
| `Collision` | `Radius` | 0.6 | Enemyとの接触判定半径 | 最低0 |
| `Stamina` | `DrainPerSecond` | 2.0 | 1秒あたりの基本スタミナ消費量 | 最低0 |
| `Stamina` | `DistanceDrainRate` | 0.08 | ロケットからの距離による追加消費倍率 | 最低0 |

スタミナの1秒あたり消費量は、概ね次の式で決まります。

```text
DrainPerSecond × (1 + ロケットからの距離 × DistanceDrainRate)
```

### Color

| 項目 | 現在値 | 変更される内容 |
|---|---|---|
| `Normal` | `(1.0, 1.0, 1.0, 1.0)` | スタミナ切れ時の色 |
| `WithStamina` | `(0.25, 0.85, 1.0, 1.0)` | スタミナが残っているときの水色 |

### ImGui: `Prototype Units`

| 表示・ボタン | 操作すると変更される値 |
|---|---|
| `Stored` | 待機中で再出撃できるユニット数を表示 |
| `Deployed` | 回収・攻撃・帰還中のユニット数を表示 |
| `Recall All` | 全ユニットを待機状態へ戻す。予約は解除され、運搬中のEnergyは現在位置へ落ちる |

---

## PrototypeEnemy

敵の生成間隔、移動、索敵、当たり判定を調整します。

### Parameter Inspector

| グループ | 項目 | 現在値 | 変更される内容 | 補正・注意点 |
|---|---|---:|---|---|
| `Spawn` | `Interval` | 4.0 | 敵を1体生成する間隔 | 最低0.1秒 |
| `Spawn` | `InitialCount` | 3 | 開始時に最外周へ配置する数 | 次回初期化時に反映。0～`MaxActiveCount` |
| `Spawn` | `MaxActiveCount` | 20 | 同時に存在できる敵数 | 1～プール容量。値を下げても既存の敵は削除しない |
| `Transform` | `Scale` | `(1.0, 1.0, 1.0)` | 敵モデルの表示倍率 | 0未満の各成分は0へ補正 |
| `Transform` | `GroundHeight` | 0.25 | 新しく出現する敵のY座標 | 既存の敵位置は変更しない |
| `Move` | `Speed` | 1.0 | 敵の移動速度 | 最低0 |
| `Target` | `SearchRadius` | 8.0 | 運搬ユニットを優先して探す半径 | 最低0 |
| `Collision` | `Radius` | 0.8 | Unit・Rocketとの接触判定半径 | 最低0 |
| `Color` | `Body` | `(1.0, 0.35, 0.35, 1.0)` | 敵の通常色 | 既存の敵にも反映 |

敵撃破時のEnergyは撃破位置で決まります。

| 撃破領域 | ドロップ |
|---|---|
| Center・Near | Small |
| NearBuffer・Middle | Medium |
| MiddleBuffer・Far・OuterBuffer・Outside | Large |

### ImGui: `Prototype Enemies`

| 表示・ボタン | 操作すると変更される値 |
|---|---|
| `Active` | 現在使用中の敵数とプール容量を表示 |
| `Targeting Carrier` | Energy運搬中ユニットを追跡している敵数を表示 |
| `Spawn Enemy` | フィールド最外周のランダム位置へ敵を1体生成 |

ゲームプレイが無効なReady・TimeUp・Pause中、または`MaxActiveCount`到達時は、生成ボタンを押しても生成されません。

---

## PrototypeLockOn

カーソル移動、選択範囲、チャージ時間、派遣時のEnergy消費を調整します。

### Cursor

| 項目 | 現在値 | 変更される内容 | 補正・注意点 |
|---|---:|---|---|
| `CursorSpeed` | 24.0 | WASD・左スティックでのカーソル移動速度 | 最低0 |
| `SelectionRadius` | 2.0 | Energy・Enemyを検索する半径 | 最低0 |
| `GroundHeight` | 0.25 | カーソルが動くXZ平面のY座標 | 即時反映 |
| `FieldEdgeMargin` | 0.5 | 最外周から内側へ設ける移動制限の余白 | 最低0 |
| `MouseMoveThreshold` | 0.01 | マウス移動として認識する最小デルタ | 最低0 |
| `ModelScale` | `(1.0, 1.0, 1.0)` | cursor.objの表示倍率 | 0未満の各成分は0へ補正 |
| `ModelHeightOffset` | 0.06 | カーソルモデルを地面から浮かせるY差 | 即時反映 |

### Charge

| 項目 | 現在値 | 変更される内容 | 補正・注意点 |
|---|---:|---|---|
| `MaxSeconds` | 3.0 | 最大チャージへ到達する保持時間 | 最低0.01秒 |
| `StartSeconds` | 0.2 | 短押しとして0消費にする猶予時間 | 0～`MaxSeconds`へ補正 |
| `MaxEnergyCost` | 30 | 最大チャージ時に要求するEnergy | 最低0 |

チャージ率と要求Energyは次の式で決まります。

```text
保持時間 <= StartSeconds:
    チャージ率 = 0

それ以外:
    チャージ率 = (保持時間 - StartSeconds) / (MaxSeconds - StartSeconds)

要求Energy = floor(MaxEnergyCost × チャージ率)
```

最大時間まで保持した場合は、必ずチャージ率100%になります。ロケットの残量が要求Energyより少ない場合、残っている分だけが実際に消費され、同じ量がユニットのスタミナになります。

### Color

| 項目 | 現在値 | 変更される内容 |
|---|---|---|
| `CursorColor` | `(0.0, 0.0, 0.0, 1.0)` | cursor.objと検索範囲の色 |
| `TargetColor` | `(1.0, 0.95, 0.25, 1.0)` | 選択対象とロケットから対象への線の色 |
| `ChargeColor` | `(1.0, 0.35, 0.20, 1.0)` | チャージ量を表す円の色 |

### ImGui: `Prototype LockOn`

このウィンドウの項目は状態表示のみで、直接変更するボタンはありません。

| 表示 | 内容 |
|---|---|
| `Move` | カーソル移動に使える入力を表示 |
| `LockOn` | ロックオンに使える入力を表示 |
| `Available Units` | 現在派遣できる待機ユニット数 |
| `Rocket Energy` | 現在のロケットEnergy |
| `Selected` | `Energy`、`Enemy`、`None`のいずれか |
| `Charge` | 現在の保持秒数と最大秒数 |
| `Charge Ratio` | 短押し猶予を除いたチャージ率 |
| `Requested Energy` | 派遣時の要求Energyと最大要求量 |

---

## PrototypeGameFlow

ゲーム開始待機時間、制限時間、各ゲームシステムの一括停止を管理します。

### Time

| 項目 | 現在値 | 変更される内容 | 反映タイミング・注意点 |
|---|---:|---|---|
| `GameDuration` | 120.0 | Playingフェーズの制限時間 | 最低0.1秒。現在進行中の残り時間は書き換えず、次回初期化時に反映 |
| `InitialDelay` | 3.0 | ReadyからPlayingまでの待機時間 | 最低0。次回初期化時に反映 |

### フェーズ

| フェーズ | 状態 |
|---|---|
| `Ready` | 開始待機中。Energy、Enemy、Unit、LockOnを停止 |
| `Playing` | 制限時間を減らし、ゲームプレイを有効化 |
| `TimeUp` | 最終Energyを固定し、ゲームプレイを停止 |
| `Launching` | ロケット発射用の予約フェーズ。現在は未実装 |
| `Result` | リザルト用の予約フェーズ。現在は未実装 |

### ImGui: `Prototype Game Flow`

| 表示・ボタン | 操作すると変更される値 |
|---|---|
| `Phase` | 現在フェーズを表示 |
| `Starts In` | Ready終了までの残り秒数を表示 |
| `Remaining` | Playing終了までの残り秒数と設定時間を表示 |
| `Final Energy` | TimeUp時点で固定したEnergyを表示 |
| `Pause` | GameFlowのタイマーとEnergy・Enemy・Unit・LockOnを停止 |
| `Resume` | Pauseで停止した処理を再開 |
| `Force Time Up` | 残り時間を0にし、現在Energyを最終値としてTimeUpへ移行 |

`Pause / Resume`と`Force Time Up`はReadyまたはPlaying中だけ表示されます。

---

## PrototypeEnergyView

ロケットの現在Energyを、0.obj～9.objの数字モデルで最大5桁表示します。専用のImGuiウィンドウはなく、Parameter Inspectorから操作します。

| 項目 | 現在値 | 変更される内容 |
|---|---|---|
| `Position` | `(1.3, 1.8, 10.0)` | カメラ座標系での左端の表示位置 |
| `Scale` | 0.25 | 全数字モデルの一様スケール |
| `DigitSpacing` | 0.55 | 隣り合う桁のX方向間隔 |
| `HideLeadingZeros` | true | trueなら不要な先頭の0を非表示 |
| `Color` | `(0.0, 0.0, 0.0, 1.0)` | 全桁へ一括適用するRGBA色 |

### Digit1～Digit5

| グループ | 対応する桁 | 調整項目 |
|---|---|---|
| `Digit1` | 一の位 | `Translate`、`Rotate` |
| `Digit2` | 十の位 | `Translate`、`Rotate` |
| `Digit3` | 百の位 | `Translate`、`Rotate` |
| `Digit4` | 千の位 | `Translate`、`Rotate` |
| `Digit5` | 万の位 | `Translate`、`Rotate` |

- `Translate`は共通の`Position`と桁間隔へ追加する個別位置補正です。
- `Rotate`はラジアン指定です。現在値は全桁ともおよそ`(1.5708, 3.1416, 0.0)`です。
- 表示可能範囲は0～99999で、それを超える値は99999として表示されます。

---

## 関連する設定ファイル

- `Resources/Json/GameData/PrototypeField.json`
- `Resources/Json/GameData/PrototypeRocket.json`
- `Resources/Json/GameData/PrototypeEnergy.json`
- `Resources/Json/GameData/PrototypeUnit.json`
- `Resources/Json/GameData/PrototypeEnemy.json`
- `Resources/Json/GameData/PrototypeLockOn.json`
- `Resources/Json/GameData/PrototypeGameFlow.json`
- `Resources/Json/GameData/PrototypeEnergyView.json`

Parameter Inspector上のグループ名は、各JSONのルート名と対応しています。
