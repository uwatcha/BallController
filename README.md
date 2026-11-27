# BallController

## 実装者の記録（User / Claude）

2026-11-22 に学習モードを終了しました。これ以降は次の分担で開発します。

- **User（自分）が手で書く:** Pawn（鉄球）の基本動作だけ
- **Claude が主体で書く:** ステージギミックなど、それ以外の機能（時間短縮のため）

### ルール

- コードファイルの **1行目** に、誰が書いたかをコメントで書く
  - `// Author: User` … 自分が書いた
  - `// Author: Claude` … Claude が書いた
  - `// Author: User + Claude` … 自分が主体で、一部を Claude が書いた（どこを書いたかは下の表に書く）
  - `// Author: UE Template` … UE が自動で作ったファイル
- 新しいファイルを作ったら、1行目のコメントを付けて、下の表にも1行足す
- 既存のファイルを Claude が書き換えたときは、表の「Claude が書いた部分」に追記する
- 誤字の修正や、名前の一括置換のような機械的な変更は、作者を変えない

### ファイルごとの記録（2026-11-25 時点）

判断の材料は、git の履歴と、Claude のプロジェクトのスレッドでのやり取りです。

| ファイル | 作者 | Claude が書いた部分 |
|---|---|---|
| `Public/SteelBallPawn.h` / `Private/SteelBallPawn.cpp` | User + Claude | `Move` のデバッグ出力を画面表示に書き換え（スレッド『鉄球Moveのデバッグ出力を画面表示に』11/07、4017d80）。Linear / Angular Damping の切り替え（`DampType` と `ApplyTuningData`・`SwitchDamp` の該当部分、11/18、92ac226）。`Die()` 冒頭のデバッグ表示（11/20）。`UCLASS` の `PrioritizeCategories`（BP で設定する項目を詳細パネルの上に出す。スレッド『い、うの部屋作成』11/25）。`ApplyBallSize()`（DataAsset の直径に合わせて拡大縮小し、底の高さを保つ）と `ApplyCameraArmLength()`（SpringArm の長さを反映）、`ApplyTuningData` からの呼び出し（スレッド『鉄球の大きさとForce上限をDataAssetで調整』11/25）。`MaxSpeedRate` / `SetMaxSpeedRate()` / `GetCurrentMaxSpeed()` と、`RemoveAccelerationAtMaxSpeed()` の比較を `GetCurrentMaxSpeed()` に変更（カーソルの距離で速度の上限を変える。同スレッド 11/25）。`IsGrounded()` と `GroundCheckMargin`（11/25 に定数から DataAsset の値に変更。スレッド『部屋Dの坂の部屋を作る』）、`GravityZ` と `ApplyGravity()`（ワールドの重力を切って DataAsset の重力を毎フレーム加える。同スレッド 11/25）、`Move()` と `Tick()` の Force / Velocity のベクトルと大きさの画面表示（同スレッド 11/25）、`Move()` で空中なら Force を 0 にする分岐と `IsGrounded` の画面表示（スレッド『空中ではForceを渡さないようにする』11/25。自分で書き直す予定、下の「自分で実装し直したいこと」参照） |
| `Public/CursorController.h` / `Private/CursorController.cpp` | User + Claude | `BindSteelBallDelegates()` / `UnbindSteelBallDelegates()`（スレッド『マウス中心点時に減衰させる機能』A案、11/18）。`UpdateCursorVector()` の `ClampScalarToMax` 表示（11/15）。`IA_DebugKill` のハンドル保存と解除、`IA_DebugRespawn` を一度だけバインドするフラグ、`OnDebugRespawn()` 冒頭のデバッグ表示（スレッド『Controllerで複数IMCを切り替えてIAをバインド』11/20、c1d3380）。`UCLASS` の `PrioritizeCategories`（BP で設定する項目を詳細パネルの上に出す。スレッド『い、うの部屋作成』11/25）。`ScaleByForceCurve()` / `GetCursorDistanceRatio()` / `ReticleCircleWidgetName` と、`UpdateCursorVector()` のサークル内の分岐（中心点に近いほど Force の上限を小さくする。スレッド『鉄球の大きさとForce上限をDataAssetで調整』11/25）。`SpeedRate` と `SetMaxSpeedRate` の呼び出し（速度の上限もカーブで変える。同スレッド 11/25） |
| `Private/ABallGameMode.cpp` | User + Claude | `RespawnPlayer()` の書き直し（Pawn がいなくても `RestartPlayer` する形、`AController` の null チェック。11/20、c1d3380） |
| `Public/ABallGameMode.h` | User | なし |
| `Public/MyPrimaryDataAsset.h` | User + Claude | `ESteelBallDampType` と `DampType`（11/18、92ac226）。`BallDiameter`・`CameraArmLength`・`ForceCurve` とコンストラクタ（スレッド『鉄球の大きさとForce上限をDataAssetで調整』11/25）。`SpeedCurve`・`bUseForceCurveForSpeed`（同スレッド 11/25）。`BumperBounceSpeed`・`BumperCooldown`（スレッド『い、うの部屋作成』11/25）。`GroundCheckMargin`（接地判定の余裕）、`GravityZ`（鉄球だけの重力）と、坂で止まる重力の計算ツール `Tool|GravityCalculator` の項目・ボタン（`Calc*`、`LoadCurrentValuesToCalculator`、`ApplyGravityFromCalculator`、GravityZ の直接入力 `CalcGravityZInput` / `ApplyGravityZInput` と初期値に戻す `ResetGravityZ`、`PostLoad`、`PostEditChangeProperty`。中身は `Private/MyPrimaryDataAsset.cpp`）（スレッド『部屋Dの坂の部屋を作る』11/25） |
| `Private/MyPrimaryDataAsset.cpp` | Claude | ファイル全体（MyPrimaryDataAsset の重力計算ツールの処理。スレッド『部屋Dの坂の部屋を作る』11/25） |
| `Public/ReticleWidget.h` / `Private/ReticleWidget.cpp` | User | なし |
| `Public/BallLog.h` | Claude | ファイル全体（`SB_LOG` マクロ。スレッド『UE_LOGに行数を出すマクロを作る』11/14、5d14391） |
| `Public/BallSwitch.h` / `Private/BallSwitch.cpp` | Claude | ファイル全体（鉄球・鉄箱で押すスイッチ。Once/Keep × 単体/グループ、3色表示、`OnActiveChanged` で扉へ通知。スレッド『あの部屋のスイッチを作る』11/23。`GetGroupName()` はスレッド『い、うの部屋作成』11/25） |
| `Public/BallDoor.h` / `Private/BallDoor.cpp` | Claude | ファイル全体（スイッチと連動して下に沈む扉。色・開く/閉じる速さ・沈む距離・待ち時間を調整可能。スレッド『あの部屋のスイッチを作る』11/24。開ききっても当たり判定を残す修正と、BallRoom から大きさを合わせる `FitSize`、グループ名でスイッチと連動する `SwitchGroupName` はスレッド『い、うの部屋作成』11/25） |
| `Public/BallRoom.h` / `Private/BallRoom.cpp` | Claude | ファイル全体（床と4枚の壁・天井を数値から組み立てる部屋。部屋の大きさ・壁の高さ/厚さ・床の厚さ・色を調整可能、壁の穴（扉・通路）を面・位置・幅・高さで指定。穴に登録した扉 BallDoor を位置・向き・大きさごと自動で合わせ、扉の下の壁をくり抜く。床の作り方は子クラスで差し替え可能（`BuildFloor`）。スレッド『い、うの部屋作成』11/24〜25。子クラス用のフック `BuildCeiling` / `GetFloorZ` / `AddWallPiece` / `GetWallBottomDepth` / `GetCenterOffset` と、壁・扉の高さを床の高さに合わせる処理、`AddBox` の向き指定、床の角丸の穴 `FloorHoles`（`FRoomFloorHole`、`AddFloorBox`、`CutFloorHoles`。Geometry Script のブーリアンで床をくり抜く）はスレッド『部屋Dの坂の部屋を作る』11/25） |
| `Public/BallSlopeRoom.h` / `Private/BallSlopeRoom.cpp` | Claude | ファイル全体（BallRoom を継承した下り坂／上り坂の部屋。アクターの位置が始点で、長さ（RoomSize.X）・傾斜の角度・向きを変えても始点は動かない。床・天井を坂に、壁・穴・扉を坂の高さに合わせる。横の壁は上端が坂と平行な斜めの形（ProceduralMesh、凸形状の当たり判定）。終点の位置を表示。坂の上の鉄球の、下る速さの上限（`bLimitDownhillSpeed` / `MaxDownhillSpeed`）と逆走禁止（`bNoReverse`）。床の穴に対応（坂の床を `AddFloorBox` で作る）。スレッド『部屋Dの坂の部屋を作る』11/25） |
| `Public/BallBumper.h` / `Private/BallBumper.cpp` | Claude | ファイル全体（当たった鉄球を中心から外向きに弾く円柱のバンパー。直径・高さ・色を調整可能、弾く速さはデータアセットで調整。スレッド『い、うの部屋作成』11/25） |
| `Public/BallIronBox.h` / `Private/BallIronBox.cpp` | Claude | ファイル全体（MF で引き寄せられる鉄箱。物理で動く箱に引き寄せ対象のタグを付けたもの。大きさ・重さ・色を調整可能。一度 MF にかけられるまで物理で動かない固定 `bLockUntilMagnet` / `UnlockByMagnet()`。スレッド『部屋Eを作る』11/27） |
| `Public/DebugCameraZoomComponent.h` / `Private/DebugCameraZoomComponent.cpp` | Claude | ファイル全体（デバッグ用カメラズーム。PlayerController に付けるコンポーネントで、キー長押しで SpringArm を縮める/伸ばし、単押しで元の長さに戻す。速さ・最短/最長・長押し判定の秒数を調整可能。Pawn と CursorController のコードは変更なし。スレッド『デバッグ用カメラズームを追加』11/24） |
| `Public/DebugDrawSubsystem.h` / `Private/DebugDrawSubsystem.cpp` | Claude | ファイル全体（座標軸ギズモ。スレッド『デバッグ軸を画面左下に表示』11/07、94130f7） |
| `Scripts/ReorganizeContent.py` | Claude | ファイル全体（Content フォルダの不要アセット削除と、種類ごとのフォルダへの移動を UE エディタで行う Python スクリプト。スレッド『Contentフォルダを整理する』11/23） |
| `BallController.h` / `BallController.cpp` / `*.Target.cs` | UE Template | なし |
| `BallController.Build.cs` | UE Template | 依存モジュールの `ProceduralMeshComponent`（坂の部屋の斜めの壁用）と `GeometryCore`・`GeometryFramework`・`GeometryScriptingCore`（床の穴用）を追加（スレッド『部屋Dの坂の部屋を作る』11/25。`.uproject` の `GeometryScripting` プラグインの有効化も Claude）。`EnhancedInput`・`UMG` は自分で追加 |

次の変更は機械的な一括置換で、作者は変えていません（Claude が実行）。

- `UE_LOG` を `SB_LOG` に置き換え（11/14）
- `AddOnScreenDebugMessage` の Key を `__LINE__` に置き換え（11/15）
- `SteelBallController` を `SteelBallPawn` に改名（11/18、88c9362）

設計や不具合の原因について Claude に相談し、ヒントをもとに自分で書いた部分は「User」としています。相談の内容は、下の「AI への相談と自分の決定の記録」にまとめています。

## AI への相談と自分の決定の記録

Claude（AI）とのチャットログと git 履歴から、分かる範囲でまとめた記録です（2026-11-08 時点）。

- 記号の意味
  - 【相談】AI に質問したこと
  - 【AI 提案】AI が出した案・回答
  - 【自分で決定】自分で決めたこと（自分の発言・コミット・コードで確かめられるもの）
  - 【不明】誰が決めたか、どの案を採ったかが記録から読み取れないもの
- 参照元の書き方
  - 「チャット (日付)」= Claude のプロジェクトチャットでのやり取り
  - 「スレッド『タイトル』(日付)」= Claude のプロジェクト内のスレッド
  - 7桁の英数字 = git のコミットハッシュ
  - 「未コミット」= 2026-11-08 時点の作業中の変更（まだコミットしていないもの）

### AI の使い方の方針

- 【自分で決定】コードは自分で書き、AI には完成形のコードを出させず、手順・API のヒント・考え方の説明だけにしてもらう「学習モード」を決め、`CLAUDE.md` に書いた（参照: e1817ee で `CLAUDE.md` を追加）
- 【自分で決定】例外として、次の3つは自分から AI に直接書かせた（下の各項目に参照元あり）
  - `ASteelBallController::Move` のデバッグ出力の書き換え
  - 座標軸ギズモ（`DebugDrawSubsystem`）の実装。コミットメッセージに「バイブコーディング」と明記した（参照: 94130f7）
  - 4017d80 のコミットメッセージの文面
- 【自分で決定】AI に「質量を調整できるように実装して」と頼んだとき、AI から「ヒントで自分で書くか、直接実装するか」を聞かれ、自分で書くほうを選んだ（参照: スレッド『CursorControllerから鉄球の質量を調整』2026-11-07）

### カーソルの位置から移動ベクトルを作る（CursorController）

- 【相談】カーソルの座標を受け取り、画面中心から見たベクトルを鉄球の移動に使いたいので、やることリストを作ってほしいと頼んだ（参照: スレッド『カーソル座標から鉄球移動ベクトルを実装』2026-11-06）
- 【AI 提案】やることリストを出した（参照: 同スレッド）
  - `GetCursorVector()` の `static` をやめる
  - 中心座標を `GetViewportSize()` で毎フレーム求める
  - -1〜1 への正規化、デッドゾーン、Y 軸の反転
  - 移動処理を `FVector2D` を受け取る関数に切り出す
- 【自分で決定】相対ベクトルを取る処理を実装し、画面サイズの変化に対応するため中心座標を毎フレーム更新することにした（参照: e1817ee、`CursorController.h` のコメント）
- 【不明】正規化とデッドゾーンは今のコードに入っていない。使わないと決めたのか、まだ手を付けていないだけなのかは記録から分からない（参照: 2262018 時点の `CursorController.cpp`）
- 【相談】`GetViewportSize` の引数の意味、参照渡しとポインタの違い、`FVector2D<int>` のコンパイルエラーの原因を聞いた（参照: チャット 2026-11-07）
- 【自分で決定】Get 関数の中で計算して Tick で値を入れる作りは、他のクラスから呼ぶときに良くないと自分で気づき、「計算する関数を private にしてメンバーに代入し、Get は値を返すだけにする」形を自分から提案した（参照: チャット 2026-11-07）
- 【AI 提案】その形に賛成した。計算して保存する関数の名前は `Calc` より `Update` がよいこと、Get に `const` を付けることを勧めた（参照: チャット 2026-11-07）
- 【自分で決定】`Update` という名前と `const` のゲッターを採用した（参照: f0d530f）
- 【相談】マウスが画面の外にあることを検知する方法を聞いた（参照: チャット 2026-11-07）
- 【AI 提案】`GetMousePosition` の戻り値（bool）で判定できると答えた。画面外のときの扱いの例として「移動ベクトルを 0 にする」を挙げた（参照: チャット 2026-11-07）
- 【自分で決定】画面外のときは入力を 0 にする処理を入れた（参照: 2262018）
- 【AI 提案】鉄球が Tick で値を読むときに1フレーム古い値を読まないよう、`AddTickPrerequisiteActor` で Tick の順番を決める方法を紹介した（参照: チャット 2026-11-07）
- 【不明】`AddTickPrerequisiteActor` は今のコードに入っていない。採らなかった理由は記録にない

### 入力の方式: Enhanced Input をやめてカーソルのベクトルを直接使う

- 【自分で決定】`SteelBallController::Move` が InputAction から入力を受け取る作りをやめ、CursorController から直接ベクトルを受け取る案を自分から出した（参照: チャット 2026-11-07）
- 【AI 提案】その案に賛成し、移動の本体（`Move(FVector2D)`）と InputAction 用の受け口（`OnMoveAction`）を分ける作りを勧めた（参照: チャット 2026-11-07）
- 【相談】せっかく作った入力処理を残したい気持ちもあるが、バグを減らすには消すべきかを聞いた（参照: チャット 2026-11-07）
- 【AI 提案】git にコミットしてから消すことを勧めた（参照: チャット 2026-11-07）
- 【自分で決定】InputAction を通す処理を消し、`Move` が `FVector2D` を受け取る形にした。`OnMoveAction` は作らなかった（参照: b6a64fb、4017d80）
- 【相談】あとでデバッグ入力に使うため、Enhanced Input を使っていた頃のコードを git 履歴から取り出してまとめてもらった（参照: スレッド『git履歴からIMCとIAの旧実装をまとめる』2026-11-07）
- 【AI 提案】4017d80 のコミットメッセージは、AI が差分を見て書いた案をそのまま使った（参照: スレッド『変更差分のコミットメッセージ作成』2026-11-07、4017d80）
  - 同じスレッドで AI は「`Tick` で `CursorController` を null チェックせずに使っているので、取得に失敗するとクラッシュする」と指摘した。null チェックは未コミットの変更で入れている

### デバッグ表示

- 【自分で決定】`ASteelBallController::Move` の中の `UE_LOG` を、CursorController と同じ書き方の画面表示に変えるよう AI に頼んだ。条件として「ほかの処理には一切触らない」と指定した（参照: スレッド『鉄球Moveのデバッグ出力を画面表示に』2026-11-07）
  - AI が `UE_LOG` 6か所を `AddOnScreenDebugMessage` に書き換えた。どのキー番号を使うか、色は AI が決めた（参照: 同スレッド、4017d80）
- 【相談】画面上で XYZ 軸の向きを表示する方法と、そうしたデバッグ機能をまとめるクラスは何を継承すべきかを聞いた（参照: チャット 2026-11-07）
- 【AI 提案】まず `UBlueprintFunctionLibrary` を勧めた（参照: チャット 2026-11-07）
- 【自分で決定】呼び出しまでクラスの中だけで完結させたいという条件を出した（参照: チャット 2026-11-07）
- 【AI 提案】その条件なら `UTickableWorldSubsystem` が合うと勧めた（参照: チャット 2026-11-07）
- 【自分で決定】ギズモの見せ方を自分で指定し、ファイルをプロジェクトに置いて AI に直接編集させた（参照: スレッド『デバッグ軸を画面左下に表示』2026-11-07）
  - 鉄球にくっついて回転するのをやめ、UI のように画面左下に出す
  - カメラは固定する予定なので、垂直の軸（Z）は出さない
  - 表示を見たあと、左下からの距離と大きさを半分にするよう追加で指示した
- 【AI 提案】基底クラスを `UWorldSubsystem` に変え、`UDebugDrawService` で Canvas に描く方式を AI が選び、コードも AI が書いた（参照: 同スレッド、94130f7）

### パラメータをエディタから調整できるようにする

- 【相談】AddForce の係数（`100.f`）を PIE 中に変えたいと相談した（参照: チャット 2026-11-07）
- 【AI 提案】係数を private のメンバー変数にして `UPROPERTY(EditAnywhere)` を付ける方法を示した（参照: チャット 2026-11-07）
- 【AI 提案】質量に関係なく同じ動きにしたいなら、`AddForce` の第3引数 `bAccelChange` を使えると紹介した（参照: チャット 2026-11-07）
- 【自分で決定】力の係数 `ForceVectorScale` と、力と加速度を切り替えるフラグ `UseAcceleration` を、エディタから編集できるようにした（参照: 3849212）
- 【自分で決定】最初は「CursorController から質量を調整したい」と頼んだ。実際には、ほかの調整値と一緒に編集できるよう `SteelBallController` 側に `Mass` を置いた（参照: スレッド『CursorControllerから鉄球の質量を調整』2026-11-07、5988c47 の `SteelBallController.h` のコメント）
- 【AI 提案】質量は `SetMassOverrideInKg` で設定し、`OnPossess` か「値が変わったときだけ」反映する方法を勧めた（参照: 同スレッド）
  - 実装では、コンストラクタと毎フレームの Tick で反映している（参照: 5988c47）
- 【相談】実装の途中で出たエラーを質問した（参照: 同スレッド）
  - メンバー関数で `this` が使えないエラー
  - `UPrimitiveComponent` を値で宣言したときの「コンストラクタがない」エラー

### Pawn をレベルに置く方式と、操作が反転する問題

- 【相談】プレイしていないときに調整値を編集する方法と、自動で生成されていた Pawn をレベルに置く方法を聞いた（参照: チャット 2026-11-07）
- 【自分で決定】Pawn をレベルに置き、GameMode の Default Pawn Class を使わない形にした（参照: スレッド『配置したPawnの操作反転を直す』2026-11-07 の自分の質問）
- 【相談】その結果、操作の向きが反転したので原因を聞いた（参照: 同スレッド）
- 【AI 提案】原因は、移動の基準が Controller の Control Rotation で、配置した Pawn の向きがそれにコピーされることだと説明した。直し方の案は次の3つ（参照: 同スレッド）
  - 1. 基準をカメラの向き（`PlayerCameraManager->GetCameraRotation()`）にする
  - 2. Pawn の向きを合わせる
  - 3. Control Rotation を自分で固定する
- 【自分で決定】1 の「カメラの向きを基準にする」を選んで直した（参照: 9ddfe98）

### キルとリスポーンのデバッグ機能、GameMode の導入（作業中）

- 【相談】Pawn をすぐキルするショートカットと、リスポーンさせるショートカットの設計を頼んだ（参照: スレッド『Pawnのキルとリポップのショートカットを設計』2026-11-07）
- 【AI 提案】次の設計を出した（参照: 同スレッド）
  - 入力は Pawn が消えても残る PlayerController（CursorController）に置く
  - Pawn に `Die()` を用意する
  - `BeginPlay` で Controller を取得していると、スポーンした Pawn ではクラッシュするという落とし穴
- 【自分で決定】あとでゲームの開始・終了・クリア判定を作る予定なので、その役割は CursorController ではなく別のクラスに分けたほうがよいのではと自分から問いかけた（参照: 同スレッド）
- 【AI 提案】その役割は GameMode が担うものだと答え、リスポーン処理も GameMode に移す案を出した（参照: 同スレッド）
- 【自分で決定】「この方針で」と承認し、作る機能と置き場所の一覧を作ってもらった（参照: 同スレッド）
- 【自分で決定】GameMode クラスを作り、CursorController にデバッグ用の IMC と IA を宣言した（参照: 3b6dae4）
- 【AI 提案】`SteelBallController.h` だけ Private フォルダにあったので、Public に移して揃えることを勧めた（参照: チャット 2026-11-07）
- 【自分で決定】`SteelBallController.h` を Public に移した（参照: 3b6dae4）
- 【自分で決定】作業中の変更（未コミット）
  - Controller を取得するタイミングを `BeginPlay` から `PossessedBy` に移し、Tick で null チェックを入れた。AI が指摘した落とし穴への対応
  - GameMode に `RespawnPlayer` を作り、`RestartPlayer` を使う形にした
  - 死亡の通知 `OnPawnDied` を、関数ではなくマルチキャストデリゲートで宣言した。AI の一覧では関数として書かれていたので、ここは自分の判断だと思われる。ただし、AI は別の回答で「下から上への通知は Delegate で」とも説明している（参照: チャット 2026-11-07「UEでスクリプト中で別のスクリプトを参照するのは一般的？」）
- 【AI 提案】GameMode を使うなら、レベルに置いた Pawn をやめて PlayerStart からスポーンする形に戻すことを勧めた（参照: スレッド『Pawnのキルとリポップのショートカットを設計』）
- 【自分で決定】それだと加速の強さなどのパラメータ調整がしにくいという懸念を自分から出した（参照: チャット 2026-11-07）
- 【AI 提案】調整用のレベルは作らず、調整値は鉄球の BP の Class Defaults に置けばよいと答えた（参照: チャット 2026-11-07）
- 【不明】最終的に PlayerStart 方式に戻したのか、レベルに Pawn を置いたままにするのかは、まだ記録から分からない

### C++ と UE の基本についての質問（AI の回答を参考にしたもの）

- 【相談】次のことを AI に聞いた（参照: チャット 2026-11-07）
  - 関数の後ろの `const` の意味、`static` と `const` を一緒に使えない理由
  - C# のようにゲッターを短く書く方法
  - Doxygen 形式（`@param` / `@return`）のコメントの書き方
  - Unity の `GetComponent` / `Find` に当たる UE の方法、クラス同士が参照し合うことは一般的か
  - `GetComponentByClass<ACursorController>` のコンパイルエラーの原因（Actor はコンポーネントではない）
  - PlayerController を GameMode に設定するときの考え方、ヘッダーを変えたあとのビルドのし直し方
  - Rider の設定（オートセーブを切る、行番号をクリックしたときブレイクポイントではなく行を選ぶ）

### ポートフォリオとしての評価を聞いたこと

- 【相談】「設計は AI に任せ、コードは自分で書いた」今の状態で、C++ の基礎力をアピールできる水準かを辛口で評価してもらった（参照: チャット 2026-11-08）
- 【AI 提案】次のように答えた（参照: 同）
  - 今のままでは弱い。面接では「なぜこの構成にしたか」を聞かれる
  - これからは自分で設計案を書いてから AI にレビューしてもらう
  - 判断の記録を README に残す
  - AI の使い方を隠さずに書く
- 【自分で決定】その提案を受けて、この記録（README.md）を AI にまとめさせた（参照: チャット 2026-11-08）

## 自分で実装し直したいこと（やることリスト）

時間の都合で Claude に書かせたが、Pawn の基本動作なので、時間があれば自分で書き直したいもの。

### 空中では Force を渡さない（接地判定）

Claude の実装（2026-11-25、スレッド『空中ではForceを渡さないようにする』）を消して、次の手順で書き直す。

- [ ] `SteelBallPawn.h` に接地判定の関数 `bool IsGrounded() const;` を宣言する
- [ ] 鉄球の中心から真下に「半径 + 少しの余裕」だけ線を飛ばす
  - 余裕は DataAsset の `GroundCheckMargin`（初期値 5cm）を `ApplyTuningData()` でコピーして使う。坂の角度 θ では 半径 × (1 / cos θ - 1) より大きくないと坂の上で外れる
  - `GetWorld()->LineTraceSingleByChannel(Hit, 始点, 終点, ECC_Visibility, Params)`
  - 始点と半径は `SphereMesh->Bounds.Origin` / `Bounds.SphereRadius`（DataAsset で直径を変えても追従する）
  - 自分自身に当たらないように `FCollisionQueryParams` で `this` を無視する
- [ ] `Move()` で、接地していない時は `ForceVector` を 0 にする
  - `bIsForceEnabled`（カーソルが中心点にあるか）とは別に判定する。このフラグは CursorController が上書きするため
- [ ] 確認用に `DrawDebugLine`（`DrawDebugHelpers.h`）で線を出し、当たったら緑・外れたら赤にしてみる
- [ ] 坂や段差の角で判定が抜けるなら、`SweepSingleByChannel` で小さい球を飛ばす形に変える
- [ ] 書き直したら、README の表から Claude の担当分を消す
