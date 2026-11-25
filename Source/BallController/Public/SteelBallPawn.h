// Author: User + Claude（User 主体。一部 Claude が実装。README.md「実装者の記録」参照）
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MyPrimaryDataAsset.h" // ESteelBallDampType を UPROPERTY で使うため
// UPROPATYなどのUE専用の記述をコンパイルできるようにするためのもの
// StudyMemo: ~~.generated.hは最後にincludeする必要がある
#include "SteelBallPawn.generated.h"

class UInputMappingContext;
class UInputAction;
class UMyPrimaryDataAsset;

UCLASS(PrioritizeCategories=("Parameter"))
class BALLCONTROLLER_API ASteelBallPawn : public APawn
{
	GENERATED_BODY()

public:
	ASteelBallPawn();

private:
	UPROPERTY()
	UPrimitiveComponent* SphereMesh;
	
	// MaxSpeedに至っている間は、入力から進行方向に加速させる成分を取り除く
	FVector2D RemoveAccelerationAtMaxSpeed(FVector2D InputVector) const;
	
	// SphereMeshのSpeedを取得する手順を関数化
	float GetSphereMeshSpeed() const;
	
	// SwitchDamp()で変更する前のDampingを定数として確保しておく
	const float INITIAL_LINEAR_DAMPING = 0.01f;
	const float INITIAL_ANGULAR_DAMPING = 0.f;
	
	// カーソルが中心点にある時にForceを0にするためのフラグ
	bool bIsForceEnabled;
	
	// [Claude] 速度の上限を MaxSpeed の何倍にするか。SetMaxSpeedRate() で設定する
	float MaxSpeedRate = 1.f;
	
	// 調整値をまとめたDataAsset (ForceVectorScale, UseAcceleration, Mass など)
	// パラメータは全てDataAssetで調整し、ApplyTuningData() で下のフィールドにコピーされる
	UPROPERTY(EditAnywhere, Category="Parameter")
	TObjectPtr<UMyPrimaryDataAsset> TuningData;
	
	// ====== TuningDataから反映される値 ======
	// このクラスでは編集せず、DataAsset側で編集する
	
	// 鉄球に加える力の倍数
	UPROPERTY(VisibleAnywhere, Category="Parameter|FromTuningData")
	float ForceVectorScale = 100.f;
	
	// 加える力を、true: 加速度, false: 力 として扱う。
	UPROPERTY(VisibleAnywhere, Category="Parameter|FromTuningData")
	bool UseAcceleration = false;
	
	// SteelBallの質量(kg)
	UPROPERTY(VisibleAnywhere, Category="Parameter|FromTuningData")
	float Mass = 500.f;
	
	// SteelBallの最高速度
	UPROPERTY(VisibleAnywhere, Category="Parameter|FromTuningData")
	float MaxSpeed = 1000.f;
	
	// カーソルがReticleの中心点にある時の減衰力
	// ゲーム開発中は、この2種類を用意する。最終的にどちらかに決める。
	UPROPERTY(VisibleAnywhere, Category="Parameter|FromTuningData")
	float LinearDamping = 10;
	UPROPERTY(VisibleAnywhere, Category="Parameter|FromTuningData")
	float AngularDamping = 10;
	// 中心点にある時に使うDampingの種類
	UPROPERTY(VisibleAnywhere, Category="Parameter|FromTuningData")
	ESteelBallDampType DampType = ESteelBallDampType::Angular;
	// [Claude] 接地判定で半径より先に余分に見る距離(cm)。DataAsset の GroundCheckMargin
	UPROPERTY(VisibleAnywhere, Category="Parameter|FromTuningData")
	float GroundCheckMargin = 5.f;
	// [Claude] 鉄球にかかる重力加速度 (cm/s²)。DataAsset の GravityZ
	UPROPERTY(VisibleAnywhere, Category="Parameter|FromTuningData")
	float GravityZ = -980.f;
	
	// TuningDataの値を上のフィールドにコピーする
	void ApplyTuningData();
	
	// Meshにエディタでのパラメータの変更を適用する
	void ApplySphereMeshMass() const;
	
	// [Claude] DataAssetの BallDiameter に合わせて鉄球を拡大縮小する。底の高さは保つ
	void ApplyBallSize();
	
	// [Claude] DataAssetの CameraArmLength を SpringArm に反映する
	void ApplyCameraArmLength();
	// [Claude] 最後に反映した CameraArmLength。値が変わった時だけ反映し、デバッグズームを毎フレーム上書きしないため
	float LastAppliedCameraArmLength = -1.f;
	
	// [Claude] 接地判定。鉄球の中心から真下に「半径 + GroundCheckMargin」だけ線を飛ばし、何かに当たれば接地中とみなす
	bool IsGrounded() const;
	
	// [Claude] ワールドの重力を切り、DataAsset の GravityZ を毎フレーム加速度として加える (鉄球だけの重力)
	void ApplyGravity() const;

	
protected:
	virtual void PossessedBy(AController*) override;
	
	virtual void BeginPlay() override;
	
	// エディタ上で配置・変更された時にも呼ばれる。詳細パネルの表示に反映するため
	virtual void OnConstruction(const FTransform& Transform) override;

public:	
	virtual void Tick(float DeltaTime) override;
	
	void Move(const FVector2D& InputVector) const;
	
	/// @param bIsActive true: 速度を減衰させる、false: 減衰状態を解除する
	/// カーソルが中心点から出た時 or 入った時に呼び出されるように、デリゲートに登録する想定
	void SwitchDamp(bool bIsActive);
	
	/// bIsForceEnabled: {true: Forceをそのまま渡す、false: Forceを0にする} この分岐のためのフラグ
	/// カーソルが中心点から出た時 or 入った時に呼び出されるように、デリゲートに登録する想定
	void SetIsForceEnabled(bool bIsEnabled) { bIsForceEnabled = bIsEnabled; }
	
	/// [Claude] 速度の上限を MaxSpeed の何倍にするか (0〜1)。CursorController がカーソルの距離から毎フレーム設定する
	void SetMaxSpeedRate(float Rate) { MaxSpeedRate = FMath::Max(0.f, Rate); }
	/// [Claude] 今の速度の上限 (MaxSpeed * MaxSpeedRate)
	float GetCurrentMaxSpeed() const { return MaxSpeed * MaxSpeedRate; }
	
	void Die();
};
