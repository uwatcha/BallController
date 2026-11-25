// Author: Claude（Claude が実装。README.md「実装者の記録」参照）
// 当たった鉄球（物理で動くもの）を、中心から外向きに弾き飛ばす円柱のバンパー。
// 弾く速さはデータアセット（UMyPrimaryDataAsset の Gimmick|Bumper）で調整する。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BallBumper.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UMyPrimaryDataAsset;

UCLASS(PrioritizeCategories=("Bumper"))
class BALLCONTROLLER_API ABallBumper : public AActor
{
	GENERATED_BODY()

public:
	ABallBumper();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

	// 弾く速さなどを読むデータアセット（鉄球と同じ MyPrimaryDataAsset を入れる）
	UPROPERTY(EditAnywhere, Category="Bumper")
	TObjectPtr<UMyPrimaryDataAsset> TuningData;

	// 円柱の直径 (cm)
	UPROPERTY(EditAnywhere, Category="Bumper", meta=(ClampMin="1.0"))
	float Diameter = 200.f;

	// 円柱の高さ (cm)。アクターの位置 = 底面の中心
	UPROPERTY(EditAnywhere, Category="Bumper", meta=(ClampMin="1.0"))
	float Height = 100.f;

	UPROPERTY(EditAnywhere, Category="Bumper|Color")
	FLinearColor Color = FLinearColor(1.f, 0.4f, 0.f);

	// 色を付ける元のマテリアル（"Color" ベクトルパラメータを持つもの）
	UPROPERTY(EditAnywhere, Category="Bumper|Color")
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY(VisibleAnywhere, Category="Bumper")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category="Bumper")
	TObjectPtr<UStaticMeshComponent> Mesh;

private:
	UFUNCTION()
	void HandleHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// 最後に弾いた時刻（連続ヒット防止）
	float LastBounceTime = -1000.f;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MeshMaterial;
};
