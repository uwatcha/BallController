// Author: User + Claude（User 主体。一部 Claude が実装。README.md「実装者の記録」参照）
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MyPrimaryDataAsset.generated.h"

/** カーソルがReticleの中心点にある時に、どちらのDampingで鉄球を減速させるか */
UENUM(BlueprintType)
enum class ESteelBallDampType : uint8
{
	// 移動速度そのものを減衰させる
	Linear,
	// 回転速度を減衰させる（転がりにブレーキをかける）
	Angular,
};

/**
 * 鉄球の操作に関する調整値をまとめたDataAsset
 * CursorController, SteelBallPawn がこのアセットを参照して値を読む
 */
UCLASS(BlueprintType)
class BALLCONTROLLER_API UMyPrimaryDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	// ===== Cursor|Force : カーソル入力から作る力 =====
	
	// カーソルがReticle外に行った時のForce
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cursor|Force", meta=(UIMin=0, UIMax=3000, ClampMin=0, ClampMax=3000))
	float MaxForce = 100.f;
	
	// カーソルがReticleの中心点にある時にブレーキのためにかける力
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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SteelBall|Physics", meta=(UIMin=1, ClampMin=1, UIMax = 5000, ClampMax = 5000, Units="kg"))
	float Mass = 500.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SteelBall|Physics", meta=(UIMin=0, ClampMin=0, UIMax = 1000, ClampMax = 1000, ForceUnits="cm/s"))
	float MaxSpeed = 200.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SteelBall|Physics", meta=(UIMin=0, ClampMin=0, UIMax = 100, ClampMax = 100))
	float LinearDamping = 10;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SteelBall|Physics", meta=(UIMin=0, ClampMin=0, UIMax = 100, ClampMax = 100))
	float AngularDamping = 10;
	
	// カーソルがReticleの中心点にある時に使うDampingの種類
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SteelBall|Physics")
	ESteelBallDampType DampType = ESteelBallDampType::Angular;
};
