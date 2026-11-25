// Author: Claude（Claude が実装。README.md「実装者の記録」参照）
// スイッチ（ABallSwitch）と連動し、条件を満たすと下に沈んで通れるようになる扉。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BallDoor.generated.h"

class ABallSwitch;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;

UCLASS(PrioritizeCategories=("Door"))
class BALLCONTROLLER_API ABallDoor : public AActor
{
	GENERATED_BODY()

public:
	ABallDoor();

	virtual void Tick(float DeltaSeconds) override;

	// 今、開く条件を満たしているか
	UFUNCTION(BlueprintPure, Category="Door")
	bool IsOpenRequested() const { return bOpenRequested; }

protected:
	virtual void BeginPlay() override;

	// エディタ上でも色を反映するため
	virtual void OnConstruction(const FTransform& Transform) override;

	// ===== エディタで設定する値 =====

	// 連動するスイッチ。グループのスイッチは、どれか1つを入れればよい（グループ全体の状態が届く）
	UPROPERTY(EditInstanceOnly, Category="Door")
	TArray<TObjectPtr<ABallSwitch>> Switches;

	// true: 入れたスイッチが全部 ON で開く / false: どれか1つが ON で開く
	UPROPERTY(EditAnywhere, Category="Door")
	bool bRequireAllSwitches = true;

	// 開くとき（下に沈むとき）の速さ (cm/秒)
	UPROPERTY(EditAnywhere, Category="Door", meta=(ClampMin="1.0"))
	float OpenSpeed = 300.f;

	// 閉じるとき（上に戻るとき）の速さ (cm/秒)。Keep スイッチから離れたときに使う
	UPROPERTY(EditAnywhere, Category="Door", meta=(ClampMin="1.0"))
	float CloseSpeed = 300.f;

	// 沈む距離 (cm)。0 なら扉の高さぶん自動で沈む
	UPROPERTY(EditAnywhere, Category="Door", meta=(ClampMin="0.0"))
	float OpenDistance = 0.f;

	// 開き始めるまでの待ち時間 (秒)
	UPROPERTY(EditAnywhere, Category="Door", meta=(ClampMin="0.0"))
	float OpenDelay = 0.f;

	// 扉の色
	UPROPERTY(EditAnywhere, Category="Door|Color")
	FLinearColor DoorColor = FLinearColor(0.1f, 0.3f, 1.f);

	// 色を付ける元のマテリアル（"Color" ベクトルパラメータを持つもの）。
	// 扉のマテリアルは毎回ここから作り直すので、メッシュのマテリアル欄は触らなくてよい
	UPROPERTY(EditAnywhere, Category="Door|Color")
	TObjectPtr<UMaterialInterface> BaseMaterial;

	// ===== コンポーネント =====

	UPROPERTY(VisibleAnywhere, Category="Door")
	TObjectPtr<USceneComponent> Root;

	// 扉の本体（当たり判定あり）。大きさは「スケール」で変える
	UPROPERTY(VisibleAnywhere, Category="Door")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

private:
	// スイッチの OnActiveChanged から呼ばれる
	UFUNCTION()
	void HandleSwitchChanged(ABallSwitch* Switch, bool bActive);

	// スイッチの状態から、開くかどうかを決め直す
	void EvaluateSwitches();

	void ApplyColor();

	bool bOpenRequested = false;

	// 開く条件を満たしてからの経過時間（OpenDelay 用）
	float OpenRequestedTime = 0.f;

	// 閉じているときの扉のワールド位置
	FVector ClosedLocation = FVector::ZeroVector;

	// 実際に沈む距離（OpenDistance が 0 なら扉の高さ）
	float ActualOpenDistance = 0.f;

	// 今どれだけ沈んでいるか (cm)
	float CurrentOffset = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DoorMaterial;
};
