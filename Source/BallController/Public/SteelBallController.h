// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CursorController.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
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
	ACursorController* CursorController;
	
	// 調整値をまとめたDataAsset (ForceVectorScale, UseAcceleration, Mass など)
	// パラメータは全てDataAssetで調整し、ApplyTuningData() で下のフィールドにコピーされる
	UPROPERTY(EditAnywhere, Category="Parameter")
	TObjectPtr<UMyPrimaryDataAsset> TuningData;
	
	// 以下はTuningDataから反映される値。ここでは編集せず、DataAsset側で編集する
	
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
	
	// TuningDataの値を上のフィールドにコピーする
	void ApplyTuningData();
	
	void UpdateSphereMeshMass();
	
protected:
	virtual void PossessedBy(AController*) override;
	virtual void BeginPlay() override;
	// エディタ上で配置・変更された時にも呼ばれる。詳細パネルの表示に反映するため
	virtual void OnConstruction(const FTransform& Transform) override;

public:	
	virtual void Tick(float DeltaTime) override;
	
	void Move(const FVector2D& InputVector);
	void Die();
};
