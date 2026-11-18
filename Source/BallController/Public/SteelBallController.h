// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
// UPROPATYなどのUE専用の記述をコンパイルできるようにするためのもの
// StudyMemo: ~~.generated.hは最後にincludeする必要がある
#include "SteelBallController.generated.h"

class UInputMappingContext;
class UInputAction;
class UMyPrimaryDataAsset;

UCLASS()
class BALLCONTROLLER_API ASteelBallController : public APawn
{
	GENERATED_BODY()

public:
	ASteelBallController();

private:
	UPROPERTY()
	UPrimitiveComponent* SphereMesh;
	
	// MaxSpeedに至っている間は、入力から進行方向に加速させる成分を取り除く
	FVector2D RemoveAccelerationAtMaxSpeed(FVector2D InputVector) const;
	
	// SphereMeshのSpeedを取得する手順を関数化
	float GetSphereMeshSpeed() const;
	
	//
	const float INITIAL_LINEAR_DAMPING = 0.01f;
	const float INITIAL_ANGULAR_DAMPING = 0.f;
	
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
	
	// マウスがReticleの中心点にある時の減衰力
	// 試しに2種作る
	UPROPERTY(VisibleAnywhere, Category="Parameter|FromTuningData")
	float LinearDamping = 10;
	UPROPERTY(VisibleAnywhere, Category="Parameter|FromTuningData")
	float AngularDamping = 10;
	
	// TuningDataの値を上のフィールドにコピーする
	void ApplyTuningData();
	
	// Meshにエディタでのパラメータの変更を適用する
	void ApplySphereMeshMass() const;
	
protected:
	virtual void PossessedBy(AController*) override;
	
	virtual void BeginPlay() override;
	
	// エディタ上で配置・変更された時にも呼ばれる。詳細パネルの表示に反映するため
	virtual void OnConstruction(const FTransform& Transform) override;

public:	
	virtual void Tick(float DeltaTime) override;
	
	void Move(const FVector2D& InputVector) const;
	
	/// @param bIsActive true: 速度を減衰させる、false: 減衰状態を解除する
	/// マウスカーソルが中心点から出た時 or 入った時に呼び出されるように、デリゲートに登録する想定
	void SwitchDamp(bool bIsActive);
	
	void Die();
};
