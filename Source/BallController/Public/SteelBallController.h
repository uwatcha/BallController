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

UCLASS()
class BALLCONTROLLER_API ASteelBallController : public APawn
{
	GENERATED_BODY()

public:
	ASteelBallController();

private:
	ACursorController* CursorController;
	
	// 鉄球に加える力の倍数
	UPROPERTY(EditAnywhere, Category="Parameter")
	float ForceVectorScale = 100.f;
	
	// 加える力を、true: 加速度, false: 力 として扱う。
	// 直接加速度を与えると、SteelBallの重さを無視することができる。
	UPROPERTY(EditAnywhere, Category="Parameter")
	bool UseAcceleration = false;
	
	// SteelBallの質量(kg)。上記のフィールドと同時に編集するため、このスクリプトから編集できるようにする。
	UPROPERTY(EditAnywhere, Category="Parameter")
	float Mass = 500.f;
	
	void UpdateSphereMeshMass();
	
protected:
	virtual void PossessedBy(AController*) override;

public:	
	virtual void Tick(float DeltaTime) override;
	
	void Move(const FVector2D& InputVector);
	void Die();
};
