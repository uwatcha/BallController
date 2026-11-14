// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MyPrimaryDataAsset.generated.h"

/**
 * 鉄球の操作に関する調整値をまとめたDataAsset
 * CursorController, SteelBallController がこのアセットを参照して値を読む
 */
UCLASS(BlueprintType)
class BALLCONTROLLER_API UMyPrimaryDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	// ===== Cursor|Force : マウス(カーソル)入力から作る力 =====
	
	// マウスがReticle外に行った時のForce
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cursor|Force", meta=(UIMin=0, UIMax=3000, ClampMin=0, ClampMax=3000))
	float MaxForce = 100.f;
	
	// マウスがReticleの中心点にある時にブレーキのためにかける力
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cursor|Force", meta=(UIMin=0, UIMax=2000, ClampMin=0, ClampMax=2000))
	float FrictionForce = 100.f;
	
	// ===== SteelBall|Movement : 鉄球に力を加える時の設定 =====
	
	// 鉄球に加える力の倍数
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SteelBall|Movement")
	float ForceVectorScale = 100.f;
	
	// 加える力を、true: 加速度, false: 力 として扱う。
	// 直接加速度を与えると、SteelBallの重さを無視することができる。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SteelBall|Movement")
	bool UseAcceleration = false;
	
	// ===== SteelBall|Physics : 鉄球の物理パラメータ =====
	
	// SteelBallの質量(kg)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SteelBall|Physics", meta=(UIMin=1, ClampMin=1, Units="kg"))
	float Mass = 500.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SteelBall|Physics", meta=(UIMin=1, ClampMin=1, Units="cm/s"))
	float MaxSpeed = 1000.f;
};
