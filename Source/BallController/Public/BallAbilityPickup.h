// Author: Claude（ファイル全体を Claude が実装。README.md「実装者の記録」参照）
// 能力の取得アイテム。透明な球の中でイラスト（板）が回る。鉄球が触れると能力（Ability）が使えるようになる。
// 取得したときの効果音・操作説明パネル（Widget）を詳細パネルで設定でき、Blueprint の「On Picked Up」イベントで演出を足せる。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BallAbilitySubsystem.h"
#include "BallAbilityPickup.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class USoundBase;
class UUserWidget;
class UMyPrimaryDataAsset;

UCLASS(PrioritizeCategories=("Pickup"))
class BALLCONTROLLER_API ABallAbilityPickup : public AActor
{
	GENERATED_BODY()

public:
	ABallAbilityPickup();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaTime) override;

protected:
	// 取得すると使えるようになる能力
	UPROPERTY(EditAnywhere, Category="Pickup")
	EBallAbility Ability = EBallAbility::MagneticForce;

	// 鉄球と同じ DataAsset を入れると、球の直径が鉄球と同じ（BallDiameter）になる。空なら下の Diameter を使う
	UPROPERTY(EditAnywhere, Category="Pickup")
	TObjectPtr<UMyPrimaryDataAsset> TuningData;

	// 球の直径 (cm)。TuningData が空のときに使う。アクターの位置 = 球の中心
	UPROPERTY(EditAnywhere, Category="Pickup", meta=(ClampMin="1.0"))
	float Diameter = 100.f;

	// 球の直径に対する、中のイラストの大きさの割合
	UPROPERTY(EditAnywhere, Category="Pickup", meta=(ClampMin="0.0", ClampMax="2.0"))
	float IconSizeRate = 0.7f;

	// オン: イラストをカメラに向け、画面の縦の軸でコインのように回す。オフ: 上向きに寝かせたまま Z 軸で回る
	UPROPERTY(EditAnywhere, Category="Pickup")
	bool bFaceCamera = true;

	// イラストが回る速さ (度/秒)。0 なら回らない
	UPROPERTY(EditAnywhere, Category="Pickup")
	float IconSpinSpeed = 90.f;

	// 画像が横や逆さに見えるときに、板の面内で回して向きを直す角度 (度)
	UPROPERTY(EditAnywhere, Category="Pickup")
	float IconImageAngle = 0.f;

	// 外側の球のマテリアル（透明なプラスチックの質感のもの）
	UPROPERTY(EditAnywhere, Category="Pickup|Look")
	TObjectPtr<UMaterialInterface> ShellMaterial;

	// 中のイラストのマテリアル（画像を貼った、両面・マスクのもの）
	UPROPERTY(EditAnywhere, Category="Pickup|Look")
	TObjectPtr<UMaterialInterface> IconMaterial;

	// ===== 取得したときの演出（空なら何もしない） =====

	// 取得したときに鳴らす音（ファンファーレなど）
	UPROPERTY(EditAnywhere, Category="Pickup|OnPickup")
	TObjectPtr<USoundBase> PickupSound;

	// 取得したときに画面に出す操作説明パネル（Widget Blueprint）
	UPROPERTY(EditAnywhere, Category="Pickup|OnPickup")
	TSubclassOf<UUserWidget> TutorialWidgetClass;

	// 操作説明パネルを出しておく秒数。0 なら出したまま（パネル側で閉じる）
	UPROPERTY(EditAnywhere, Category="Pickup|OnPickup", meta=(ClampMin="0.0", Units="s"))
	float TutorialDisplaySeconds = 0.f;

	// 取得したときに Blueprint で演出を足すためのイベント。Picker = 取得した鉄球
	UFUNCTION(BlueprintImplementableEvent, Category="Pickup")
	void OnPickedUp(APawn* Picker);

	UPROPERTY(VisibleAnywhere, Category="Pickup")
	TObjectPtr<USceneComponent> Root;

	// 外側の透明な球（当たり判定なし）
	UPROPERTY(VisibleAnywhere, Category="Pickup")
	TObjectPtr<UStaticMeshComponent> ShellMesh;

	// 中のイラストを貼る板（エンジン標準の Plane。上を向いている）
	UPROPERTY(VisibleAnywhere, Category="Pickup")
	TObjectPtr<UStaticMeshComponent> IconMesh;

private:
	bool bPickedUp = false;

	// 今の回転の角度 (度)
	float SpinAngle = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> TutorialWidget;

	float GetDiameter() const;
	void PickUp(APawn* Picker);
	void UpdateIconRotation(float DeltaTime);
};
