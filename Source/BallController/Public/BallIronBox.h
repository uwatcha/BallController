// Author: Claude（ファイル全体を Claude が実装。README.md「実装者の記録」参照）
// マグネティックフォースで引き寄せられる鉄箱。物理で動く箱に、引き寄せ対象のアクタータグを付けただけのもの。
// 大きさ・重さ・色を詳細パネルで変えられる。スイッチ（BallSwitch）の上に置くと押せる。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BallIronBox.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS(PrioritizeCategories=("IronBox"))
class BALLCONTROLLER_API ABallIronBox : public AActor
{
	GENERATED_BODY()

public:
	ABallIronBox();

	virtual void OnConstruction(const FTransform& Transform) override;

	// MF にかけられたときに MagneticForceComponent が呼ぶ。固定を解いて物理で動くようにする（一度解いたら戻らない）
	void UnlockByMagnet();

	// 固定中か（まだ一度も MF にかけられていない）
	bool IsLockedByMagnet() const { return bLocked; }

protected:
	virtual void BeginPlay() override;

	// オン: 一度 MF にかけられるまで、物理で動かない（鉄球で押しても動かない、ただの壁として当たる）
	UPROPERTY(EditAnywhere, Category="IronBox")
	bool bLockUntilMagnet = true;

	// 箱の大きさ (cm)。アクターの位置 = 箱の中心。アクターのスケールは使わず、ここで大きさを決める
	UPROPERTY(EditAnywhere, Category="IronBox", meta=(ClampMin="1.0"))
	FVector Size = FVector(100.f, 100.f, 100.f);

	// 箱の重さ (kg)。MF の引っ張られ方には影響しない（加速度で引っ張るため）。スイッチの MinMassKg と比べるのに使う
	UPROPERTY(EditAnywhere, Category="IronBox", meta=(ClampMin="0.1"))
	float MassKg = 100.f;

	// 引き寄せ対象のアクタータグ。DataAsset の MFTargetTag と同じ名前にする
	UPROPERTY(EditAnywhere, Category="IronBox")
	FName MagneticTag = TEXT("Magnetic");

	UPROPERTY(EditAnywhere, Category="IronBox|Color")
	FLinearColor Color = FLinearColor(0.35f, 0.35f, 0.4f);

	// 色を付ける元のマテリアル（"Color" ベクトルパラメータを持つもの）
	UPROPERTY(EditAnywhere, Category="IronBox|Color")
	TObjectPtr<UMaterialInterface> BaseMaterial;

	// 物理で動く箱。物理で動かすため、ルートにしている
	UPROPERTY(VisibleAnywhere, Category="IronBox")
	TObjectPtr<UStaticMeshComponent> Mesh;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MeshMaterial;

	bool bLocked = false;

	// 前回付けたタグ (MagneticTag を書き換えたときに外すため)
	UPROPERTY()
	FName LastMagneticTag;
};
