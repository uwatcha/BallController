// Author: User + Claude（User 主体。一部 Claude が実装。README.md「実装者の記録」参照）
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Curves/CurveFloat.h" // FRuntimeFloatCurve (DataAsset内で編集できるカーブ)
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
	UMyPrimaryDataAsset()
	{
		// [Claude] ForceCurve の初期値は直線 (0,0)→(1,1)。外周で MaxForce になり、外周より外と連続する
		FRichCurve* Curve = ForceCurve.GetRichCurve();
		Curve->AddKey(0.f, 0.f);
		Curve->AddKey(1.f, 1.f);
		// [Claude] SpeedCurve の初期値も同じ直線
		FRichCurve* Speed = SpeedCurve.GetRichCurve();
		Speed->AddKey(0.f, 0.f);
		Speed->AddKey(1.f, 1.f);
	}
	
	// ===== Cursor|Force : カーソル入力から作る力 =====
	
	// カーソルがReticle外に行った時のForce
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cursor|Force", meta=(UIMin=0, UIMax=3000, ClampMin=0, ClampMax=3000))
	float MaxForce = 100.f;
	
	// カーソルがReticleの中心点にある時にブレーキのためにかける力
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cursor|Force", meta=(UIMin=0, UIMax=2000, ClampMin=0, ClampMax=2000))
	float FrictionForce = 100.f;
	
	// [Claude] 外周円の内側で、Forceの上限を MaxForce の何倍にするかのカーブ
	// 横軸: カーソルの距離 (0 = 中心点, 1 = 外周円)
	// 縦軸: MaxForce にかける倍率 (0 〜 1)。1 で MaxForce と同じ
	// 外周円より外では、このカーブは使わず MaxForce をそのまま使う
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cursor|Force", meta=(XAxisName="Distance (0=Center, 1=Circle)", YAxisName="MaxForce Rate"))
	FRuntimeFloatCurve ForceCurve;
	
	// [Claude] true: 速度の上限にも Force Curve を使う (Speed Curve は使わない)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cursor|Force")
	bool bUseForceCurveForSpeed = false;
	
	// [Claude] 外周円の内側で、速度の上限を MaxSpeed の何倍にするかのカーブ
	// 横軸: カーソルの距離 (0 = 中心点, 1 = 外周円)
	// 縦軸: MaxSpeed にかける倍率 (0 〜 1)。1 で MaxSpeed と同じ
	// 外周円より外では、MaxSpeed をそのまま使う
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cursor|Force", meta=(EditCondition="!bUseForceCurveForSpeed", XAxisName="Distance (0=Center, 1=Circle)", YAxisName="MaxSpeed Rate"))
	FRuntimeFloatCurve SpeedCurve;
	
	// ===== SteelBall|Movement : 鉄球に力を加える時の設定 =====
	
	// 鉄球に加える力の倍数
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SteelBall|Movement")
	float ForceVectorScale = 100.f;
	
	// 加える力を、true: 加速度, false: 力 として扱う。
	// 直接加速度を与えると、SteelBallの重さを無視することができる。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SteelBall|Movement")
	bool UseAcceleration = false;
	
	// ===== SteelBall|Size : 鉄球の大きさ [Claude] =====
	
	// 鉄球の直径(cm)。Meshと当たり判定が一緒に拡大縮小される
	// 大きさを変えた時は、鉄球の底の高さが変わらないように位置を上下にずらす (床を突き抜けないようにするため)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SteelBall|Size", meta=(UIMin=10, ClampMin=10, UIMax=1000, ClampMax=1000, Units="cm"))
	float BallDiameter = 100.f;
	
	// ===== Camera : カメラ [Claude] =====
	
	// カメラと鉄球の距離 (SpringArm の Target Arm Length)
	// 0 の時は BP_SteelBallPawn の SpringArm に設定した値をそのまま使う
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera", meta=(UIMin=0, ClampMin=0, UIMax=10000, ClampMax=10000, Units="cm"))
	float CameraArmLength = 0.f;
	
	// ===== SteelBall|Physics : 鉄球の物理パラメータ =====
	
	// SteelBallの質量(kg)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SteelBall|Physics", meta=(UIMin=1, ClampMin=1, UIMax = 5000, ClampMax = 5000, Units="kg"))
	float Mass = 500.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SteelBall|Physics", meta=(UIMin=0, ClampMin=0, UIMax = 5000, ClampMax = 5000, ForceUnits="cm/s"))
	float MaxSpeed = 200.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SteelBall|Physics", meta=(UIMin=0, ClampMin=0, UIMax = 100, ClampMax = 100))
	float LinearDamping = 10;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SteelBall|Physics", meta=(UIMin=0, ClampMin=0, UIMax = 100, ClampMax = 100))
	float AngularDamping = 10;
	
	// カーソルがReticleの中心点にある時に使うDampingの種類
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SteelBall|Physics")
	ESteelBallDampType DampType = ESteelBallDampType::Angular;
};
