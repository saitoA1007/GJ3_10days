# ロケット打ち上げゲーム Prototype 仕様説明書

## 1. 文書の目的

本書は`ProtoScene`に実装されているゲームプロトタイプの仕様を、企画・プログラム・調整担当者が共通認識を持てるようにまとめたものです。

記載内容は現在の実装を基準とし、未実装の構想は「未実装仕様」として区別します。Parameter InspectorとImGuiの詳しい操作方法は[PrototypeImGuiGuide.md](PrototypeImGuiGuide.md)を参照してください。

## 2. ゲーム概要

| 項目 | 内容 |
|---|---|
| 仮タイトル | ロケット打ち上げゲーム Prototype |
| ジャンル | タワーディフェンス風のエネルギー回収ゲーム |
| 描画 | 3Dモデル |
| ゲーム操作 | XZ平面上の2Dカーソル操作 |
| 目的 | 制限時間内にEnergyをロケットへ集め、最終Energyを増やす |
| 防衛要素 | 外周から接近するEnemyによるEnergy損失をUnitで防ぐ |
| 現在の終了条件 | 制限時間が0になるとTimeUpになり、ゲームプレイが停止する |

最終Energyを使ったロケット発射・高度計算・リザルトは、フェーズだけが用意されており、現在は未実装です。

## 3. シーン構成

起動時のデフォルトシーンは`Proto`です。`ProtoScene`が次のシステムを生成し、`GameObjectManager`が所有します。

| システム | 主な責務 |
|---|---|
| `Field` | 7層の円形フィールド表示と距離帯判定 |
| `Rocket` | 中央ロケットの表示、Energy保管、Enemy到達時の損失 |
| `EnergySpawner` | Energyのプール管理、空中生成、落下、検索 |
| `UnitManager` | Unitのプール管理、派遣、再出撃 |
| `EnemyManager` | Enemyのプール管理、外周生成、更新、検索 |
| `LockOnController` | カーソル移動、対象選択、チャージ、Unit派遣 |
| `GameFlowController` | 開始待機、制限時間、TimeUp、一括停止 |
| `EnergyView` | 数字モデルによる現在Energy表示 |

`ProtoScene`が保持する各システムへのポインタは非所有です。個々のEnemy、Energy、Unitは、それぞれのManagerが`unique_ptr`で所有します。

## 4. 座標と操作

### 4.1 座標系

- ゲームプレイはXZ平面上で行います。
- Y座標はモデルの高さ、落下、表示の重なり回避に使用します。
- 距離判定は基本的にYを無視したXZ距離で行います。
- フィールド中心の初期位置は`(0.0, 0.0, 0.0)`です。

### 4.2 入力

| 操作 | マウス | キーボード | ゲームパッド |
|---|---|---|---|
| カーソル移動 | マウス移動 | `WASD` | 左スティック |
| ロックオン開始・チャージ | 左ボタン | `Space` | Aボタン |
| 派遣決定 | ボタンを離す | キーを離す | Aボタンを離す |

マウスが移動したフレームはマウス座標を優先します。それ以外はWASDと左スティックの入力を合成してカーソルを移動します。

## 5. ゲーム進行仕様

### 5.1 フェーズ

| フェーズ | 実装状態 | 動作 |
|---|---|---|
| `Ready` | 実装済み | 開始待機時間を減らす。Energy、Enemy、Unit、LockOnは停止 |
| `Playing` | 実装済み | 制限時間を減らし、各ゲームシステムを稼働 |
| `TimeUp` | 実装済み | 残り時間を0にし、最終Energyを保存してゲームプレイを停止 |
| `Launching` | 未実装 | 将来のロケット発射演出用 |
| `Result` | 未実装 | 将来の高度・スコア表示用 |

現在の時間設定は次のとおりです。

| 設定 | 現在値 |
|---|---:|
| 開始待機時間 | 1秒 |
| ゲーム制限時間 | 120秒 |

### 5.2 一時停止

ImGuiの`Pause`を押すと、GameFlowの時間と次の処理が停止します。

- Energyの落下と自動生成
- Enemyの生成・索敵・移動
- Unitの移動とスタミナ消費
- カーソル操作とロックオン

`Resume`で再開します。`Force Time Up`は残り時間を0にし、その時点のRocket Energyを最終値として保存します。

## 6. フィールド仕様

### 6.1 7層構成

フィールドは同じ円形モデル`fieldCircle.obj`を7枚重ねて表現します。内側の円ほどY座標をわずかに高くし、面のちらつきを防ぎます。

| 順序 | 領域 | 外周半径 | 空からのEnergy生成 | 主な用途 |
|---:|---|---:|---|---|
| 1 | `Center` | 5 | なし | Rocketを配置する中心領域 |
| 2 | `Near` | 10 | Small | 近距離の回収領域 |
| 3 | `NearBuffer` | 15 | なし | NearとMiddleの生成間隔 |
| 4 | `Middle` | 20 | Medium | 中距離の回収領域 |
| 5 | `MiddleBuffer` | 25 | なし | MiddleとFarの生成間隔 |
| 6 | `Far` | 30 | Large | 遠距離の回収領域 |
| 7 | `OuterBuffer` | 35 | なし | Enemyの出現円周 |

最外周半径より外側は`Outside`として扱います。

### 6.2 領域判定

フィールド中心から対象座標までのXZ距離を求め、内側の半径から順に比較します。境界線上は内側の領域に含まれます。

半径を調整する場合は、必ず次の大小関係を維持する必要があります。

```text
Center < Near < NearBuffer < Middle < MiddleBuffer < Far < OuterBuffer
```

## 7. Rocket仕様

### 7.1 配置

- フィールド中央に`rocket.obj`を配置します。
- 現在の位置は`(0.0, 0.25, 0.0)`です。
- Enemy到達判定には半径2.5の範囲を使用します。

### 7.2 Energy管理

| 項目 | 現在値 |
|---|---:|
| ゲーム開始時Energy | 100 |
| Enemy 1体の到達による損失 | 10 |

Energyは常に0以上です。消費要求が現在値を超えた場合は、残っている量だけを消費します。加算で`int32_t`の上限を超える場合は上限値で止まります。

Energyの変更理由は次の5種類に分けて記録されます。

- UnitによるEnergy配達
- Unitへのスタミナ割り当て
- EnemyのRocket到達
- デバッグ操作
- リセット

## 8. Energy仕様

### 8.1 サイズと価値

| サイズ | 自然生成領域 | Scale | Rocketへの加算量 |
|---|---|---:|---:|
| `Small` | Near | 0.5 | 10 |
| `Medium` | Middle | 0.7 | 25 |
| `Large` | Far | 1.0 | 50 |

### 8.2 自然生成

- 開始時にNear、Middle、Farへ1個ずつ生成します。
- Playing中は5秒ごとにNear、Middle、Farのいずれかをランダムに選び、1個生成します。
- 自然生成される同時存在上限は12個です。
- 生成位置は各領域の円環内で、面積に対して一様になるよう抽選します。
- 現在は地面から1.0上空に生成し、毎秒5.0の速度で落下します。
- 落下中のEnergyはロックオンできません。
- 着地後は移動せず、ロックオン可能になります。

### 8.3 状態遷移

```text
Inactive
  ├─ 空中生成 ─> Falling ─> OnGround
  └─ Enemyドロップ ──────> OnGround

OnGround ─> Reserved ─> Carried ─> 納品 ─> Inactive
                         └─ Unit敗北・回収 ─> OnGround
```

| 状態 | 内容 |
|---|---|
| `Inactive` | プール内で未使用 |
| `Falling` | 空中から落下中 |
| `OnGround` | 地面にあり、ロックオン可能 |
| `Reserved` | 派遣Unitが予約済みで、他のUnitは選択不可 |
| `Carried` | Unitの頭上に追従して移動中 |

## 9. Unit仕様

### 9.1 基本仕様

- 現在の参加Unit数は5体です。
- UnitはRocket内から出撃します。
- 撃破・相打ち・納品後にStoredへ戻り、同じ個体を再出撃できます。
- Unit自体が永久に失われることはありません。
- スタミナが残っているUnitは水色、スタミナ切れのUnitは通常色で表示します。

### 9.2 派遣先

Unitは次のどちらかへ派遣できます。

1. 地面にある未予約のEnergy
2. アクティブで未予約のEnemy

対象の予約に失敗した場合は出撃せず、Rocket Energyも消費しません。

### 9.3 移動速度とスタミナ

| 状態 | 現在の移動速度 |
|---|---:|
| スタミナあり | 5.0 |
| スタミナなし | 1.5 |

派遣時にRocketから実際に消費できたEnergy量が、そのUnitの初期スタミナになります。

スタミナの1秒あたり消費量は次の式です。

```text
消費量/秒 = DrainPerSecond × (1 + Rocketからの距離 × DistanceDrainRate)
```

現在値は次のとおりです。

```text
DrainPerSecond = 2.0
DistanceDrainRate = 0.1
```

Rocketから遠いほどスタミナ消費が増加します。スタミナが0になってもUnitは停止せず、通常速度で行動を続けます。

### 9.4 Energy回収

1. Unitが対象Energyを予約します。
2. RocketからUnitへチャージ量に応じたEnergyを割り当てます。
3. Unitが対象位置へ移動します。
4. 回収半径0.5以内に入ると、Energyを頭上へ載せます。
5. Rocketへ戻り、納品半径1.6以内に入るとEnergyのValueを加算します。
6. UnitはStoredへ戻ります。

### 9.5 状態遷移

```text
Stored
  ├─ Energyへ派遣 ─> MovingToEnergy ─> ReturningToRocket ─> Stored
  └─ Enemyへ派遣 ──> MovingToEnemy
                         ├─ 勝利 ─> ReturningToRocket ─> Stored
                         └─ 相打ち ───────────────────> Stored
```

## 10. Enemy仕様

### 10.1 生成

- Enemyは1種類のみです。
- 開始時に1体生成します。
- Playing中は3秒ごとに1体生成します。
- 同時存在上限は20体です。
- FieldのOuterBuffer円周上からランダムな角度で出現します。
- 現在の移動速度は毎秒1.0です。

### 10.2 ターゲット選択

通常時はRocketへ向かいます。

Enemyの位置から半径8.0以内にEnergy運搬中のUnitが存在する場合は、その中で最も近いUnitを優先して追跡します。現在の対象が運搬を続けている間は追跡を継続し、対象が消えた、納品した、倒された場合は再選択します。

### 10.3 Enemyと運搬Unitの接触

- 運搬Unitは倒され、即座にStoredへ戻ります。
- 運搬していたEnergyは接触地点の地面へ落ちます。
- Enemyは倒れず、次フレームからRocketまたは別の運搬Unitを追跡します。

### 10.4 EnemyとRocketの接触

- Rocket Energyを10減らします。
- Enemyは非アクティブになり、再生成に使用できる状態へ戻ります。

### 10.5 Enemyと攻撃Unitの戦闘

接触判定を行う直前までUnitのスタミナを消費し、接触時点の残量で勝敗を決めます。

| Unitの状態 | 結果 |
|---|---|
| スタミナが0より大きい | Unitの勝利。Enemyが落としたEnergyをそのまま運搬してRocketへ戻る |
| スタミナが0 | 相打ち。EnemyとUnitはフィールドから消え、Energyだけが撃破地点に残る |

Unitは相打ちしてもStoredへ戻るため、再出撃できます。

### 10.6 撃破時のEnergyドロップ

Enemyを倒した位置のフィールド領域でサイズを決定します。

| 撃破位置 | ドロップ |
|---|---|
| Center・Near | Small |
| NearBuffer・Middle | Medium |
| MiddleBuffer・Far・OuterBuffer・Outside | Large |

距離に直すと、現在の半径では次の判定になります。

- 中心から10以下: Small
- 中心から10より大きく20以下: Medium
- 中心から20より大きい: Large

## 11. LockOn・Cursor仕様

### 11.1 カーソル範囲

- カーソルはField最外周から`FieldEdgeMargin`を引いた円内に制限します。
- 現在の選択半径は2.0です。
- `cursor.obj`の表示Scaleは`SelectionRadius × ModelScale`です。
- SelectionRadiusを変更すると、検索範囲とカーソルモデルが同じ比率で変化します。

### 11.2 対象選択

選択半径内から、次の条件を満たす最も近い対象を選びます。

- `OnGround`状態のEnergy
- アクティブかつ攻撃予約されていないEnemy

EnergyとEnemyの両方が範囲内にいる場合は距離を比較し、同距離ならEnemyを選択します。チャージ開始後は対象を固定し、対象が無効になった場合は派遣せずキャンセルします。

### 11.3 チャージとEnergy消費

現在の設定値は次のとおりです。

| 設定 | 現在値 |
|---|---:|
| 短押し猶予`StartSeconds` | 0.2秒 |
| 最大チャージ時間`MaxSeconds` | 3.0秒 |
| 最大要求Energy`MaxEnergyCost` | 30 |

チャージ率は次の式で計算します。

```text
保持時間 <= StartSeconds:
    チャージ率 = 0

StartSeconds < 保持時間 < MaxSeconds:
    チャージ率 = (保持時間 - StartSeconds) / (MaxSeconds - StartSeconds)

保持時間 >= MaxSeconds:
    チャージ率 = 1
```

派遣時の要求Energyは次の式で、小数点以下を切り捨てます。

```text
要求Energy = floor(MaxEnergyCost × チャージ率)
```

0.2秒以内の単発クリックではEnergyを消費せず、スタミナ0のUnitを派遣します。Rocket Energyが要求量より少ない場合は、残量だけを消費します。

## 12. 画面表示仕様

### 12.1 Cursor

`cursor.obj`をXZ平面上のカーソル位置へ表示します。カーソルモデルは選択半径と連動して拡縮します。

デバッグ描画では次の情報も表示します。

- 選択範囲の円
- 選択対象の円
- Rocketから選択対象への線
- チャージ率に応じて拡大する円

### 12.2 EnergyView

Rocketの現在Energyを、`Resources/Models/Number/0.obj`～`9.obj`で表示します。

- 最大5桁
- 表示範囲は0～99999
- 99999を超える場合は99999として表示
- 先頭の不要な0は非表示
- 表示位置はカメラ座標系基準
- 全桁の色をまとめて変更可能
- 各桁の位置・回転を個別調整可能

### 12.3 ゲーム時間

制限時間とフェーズは`Prototype Game Flow`のImGuiウィンドウで確認できます。通常ゲーム画面上のカウントダウン、残り時間、TimeUp表示は未実装です。

## 13. オブジェクトプール仕様

実行中の頻繁な生成・破棄を避けるため、Energy、Unit、Enemyは固定プールで再利用します。

| オブジェクト | プール容量 | 現在の参加・存在上限 |
|---|---:|---:|
| Energy | 64 | 自然生成上限12。Enemyドロップは空きプールが必要 |
| Unit | 16 | 参加数5 |
| Enemy | 64 | 同時存在上限20 |

プールに空きがない場合、追加生成は失敗します。Enemy撃破時にEnergyプールが満杯の場合は、ドロップを生成できません。

## 14. 更新順序

明示的に指定している更新順は次のとおりです。

| Order | システム | 意図 |
|---:|---|---|
| 0 | GameFlowController | 先にTimeUpや停止状態を確定する |
| 10 | EnergySpawner | Energyの落下・生成を更新する |
| 20 | UnitManager | Unitの移動・回収・戦闘を更新する |
| 25 | EnemyManager | Unit更新後の位置を使ってEnemyを更新する |
| 30 | LockOnController | 更新後のEnergy・Enemyから選択対象を決める |
| 40 | EnergyView | 全処理後のRocket Energyを表示へ反映する |

## 15. 調整データ

各システムはRegister式のParameter Inspectorへ設定を公開し、次のJSONへ保存します。

- `Resources/Json/GameData/PrototypeField.json`
- `Resources/Json/GameData/PrototypeRocket.json`
- `Resources/Json/GameData/PrototypeEnergy.json`
- `Resources/Json/GameData/PrototypeUnit.json`
- `Resources/Json/GameData/PrototypeEnemy.json`
- `Resources/Json/GameData/PrototypeLockOn.json`
- `Resources/Json/GameData/PrototypeGameFlow.json`
- `Resources/Json/GameData/PrototypeEnergyView.json`

設定値の意味、反映タイミング、ImGuiボタンについては[PrototypeImGuiGuide.md](PrototypeImGuiGuide.md)に記載しています。

## 16. 未実装・仕様検討中

### 16.1 未実装

- TimeUpからLaunchingへの遷移
- 最終Energyから飛行速度・高度を算出する式
- Rocketの上昇アニメーション
- 最高到達高度の計測
- LaunchingからResultへの遷移
- 最終Energy・高度を表示するResult画面
- リトライ・別シーンへの遷移
- 通常画面上の開始カウントダウン
- 通常画面上の残り時間・TimeUp表示

### 16.2 仕様検討中

- 派遣されていないStored状態のUnitへ、自動迎撃や自動回収などの役割を持たせるか
- Rocketの最終Energyと飛行高度のバランス式
- 最低飛行高度と最大飛行高度

## 17. 基本動作の確認項目

| 確認項目 | 期待結果 |
|---|---|
| Energyを単発クリックで選択して離す | Rocket Energyを消費せずUnitが通常速度で出撃する |
| Energyを長押しして離す | チャージ率に応じたEnergyを消費し、水色のUnitが高速で出撃する |
| UnitがEnergyへ到達する | Energyを頭上へ載せてRocketへ帰還する |
| UnitがRocketへ戻る | サイズに応じたValueがRocket Energyへ加算され、Unitが再出撃可能になる |
| Enemyの索敵範囲に運搬Unitが入る | EnemyがRocketより運搬Unitを優先する |
| Enemyが運搬Unitへ接触する | Unitが再出撃可能になり、Energyがその場へ落ち、Enemyは移動を続ける |
| スタミナありの攻撃UnitがEnemyへ接触する | Enemyを倒し、距離帯に応じたEnergyを運搬して帰還する |
| スタミナなしの攻撃UnitがEnemyへ接触する | 相打ちになり、Energyだけが撃破地点へ残る |
| EnemyがRocketへ到達する | Rocket Energyが10減り、Enemyが消える |
| 制限時間が0になる | 最終Energyが保存され、Energy・Unit・Enemy・LockOnが停止する |

## 18. 関連資料

- [PrototypeImGuiGuide.md](PrototypeImGuiGuide.md): Parameter InspectorとImGui操作の詳細
- [PrototypeRemainingTasks.txt](PrototypeRemainingTasks.txt): 未実装項目の簡易一覧
- `Application/Prototype/`: Prototype固有コード
- `Application/Scene/ProtoScene.cpp`: 各システムの生成と接続
