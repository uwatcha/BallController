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
	
	// [Claude] 接地判定で、鉄球の中心から真下に「半径 + この距離」まで床を探す (cm)
	// 大きくすると、崖から少し踏み外した時や小さく跳ねた時も「接地中」のままになり、操作が効く
	// 坂の上では真下の床が半径より遠くなるので、坂の角度 θ に対して 半径 × (1 / cos θ - 1) より大きくする
	// (直径 100cm の鉄球なら 30度で 8cm 以上、45度で 21cm 以上)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SteelBall|Movement", meta=(UIMin=0, ClampMin=0, UIMax=500, ClampMax=500, Units="cm"))
	float GroundCheckMargin = 5.f;
	
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
	
	// ===== Gimmick|Bumper : バンパー [Claude] =====
	
	// バンパーに当たった鉄球を、バンパーの中心から外向きに弾き飛ばす速さ (cm/s)。鉄球の重さに関係なくこの速さになる
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Gimmick|Bumper", meta=(UIMin=0, ClampMin=0, UIMax=5000, ClampMax=5000, ForceUnits="cm/s"))
	float BumperBounceSpeed = 1500.f;
	
	// 一度弾いてから、次に弾けるようになるまでの時間 (秒)。連続で何度も弾かれるのを防ぐ
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Gimmick|Bumper", meta=(UIMin=0, ClampMin=0, UIMax=2, ClampMax=2, Units="s"))
	float BumperCooldown = 0.1f;
	
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
	
	// [Claude] 鉄球にかかる重力加速度 (cm/s²)。マイナスが下向き。UE の標準は -980
	// 鉄球だけに効く (鉄箱などほかの物は、ワールドの重力のまま)
	// 下の「Tool|GravityCalculator」のボタンで、坂で止まる値を計算して入れられる
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SteelBall|Physics", meta=(UIMin=-10000, UIMax=0))
	float GravityZ = -980.f;
	
	// ===== Tool|GravityCalculator : 坂で止まる重力を計算するツール [Claude] =====
	// DataAsset を開いた時 (読み込んだ時) に、上の本来の値が入る。ここを書き換えても本来の値は変わらない。
	// 「Apply Gravity From Calculator」ボタンを押した時だけ、MaxSpeed / MaxForce / Mass と GravityZ に書き込む
	
	// 計算用の最高速度 (cm/s)。止まる重力の計算には使わない (ボタンで MaxSpeed に書き込むだけ)
	UPROPERTY(EditDefaultsOnly, Transient, Category="Tool|GravityCalculator", meta=(UIMin=0, ClampMin=0, UIMax=5000, ClampMax=5000, ForceUnits="cm/s"))
	float CalcMaxSpeed = 200.f;
	
	// 計算用の MaxForce
	UPROPERTY(EditDefaultsOnly, Transient, Category="Tool|GravityCalculator", meta=(UIMin=0, ClampMin=0, UIMax=3000, ClampMax=3000))
	float CalcMaxForce = 100.f;
	
	// 計算用の鉄球の質量 (kg)。UseAcceleration がオンの時は計算に使わない (重さに関係なく同じ加速になるため)
	UPROPERTY(EditDefaultsOnly, Transient, Category="Tool|GravityCalculator", meta=(UIMin=1, ClampMin=1, UIMax=5000, ClampMax=5000, Units="kg"))
	float CalcMass = 500.f;
	
	// 対象の坂の角度 (度)。この坂で、MaxForce で押しても登れずに止まる重力を計算する
	UPROPERTY(EditDefaultsOnly, Transient, Category="Tool|GravityCalculator", meta=(UIMin=1, ClampMin=1, UIMax=80, ClampMax=80))
	float CalcSlopeAngle = 30.f;
	
	// 計算した重力にかける倍率。1 でちょうど止まる (釣り合う)。1 より大きいと押しても少しずつ下がる
	UPROPERTY(EditDefaultsOnly, Transient, Category="Tool|GravityCalculator", meta=(UIMin=0.5, ClampMin=0.1, UIMax=2))
	float CalcGravityRate = 1.f;
	
	// 上の入力から計算した重力 (表示だけ。ボタンを押すと GravityZ に入る)
	UPROPERTY(VisibleDefaultsOnly, Transient, Category="Tool|GravityCalculator")
	float CalcResultGravityZ = -980.f;
	
	// 重力を直接入力する欄 (cm/s²)。開いた時点の GravityZ が入る。「Apply Gravity Z Input」ボタンで GravityZ に書き込む
	UPROPERTY(EditDefaultsOnly, Transient, Category="Tool|GravityCalculator", meta=(UIMin=-10000, UIMax=0))
	float CalcGravityZInput = -980.f;
	
	// 上の「Calc Gravity Z Input」の値を GravityZ に書き込む
	UFUNCTION(CallInEditor, Category="Tool|GravityCalculator")
	void ApplyGravityZInput();
	
	// GravityZ を初期値 (プロジェクト設定の Default Gravity Z。変えていなければ -980) に戻す
	UFUNCTION(CallInEditor, Category="Tool|GravityCalculator")
	void ResetGravityZ();
	
	// 上の本来の値 (MaxSpeed / MaxForce / Mass) を、計算ツールの入力欄に読み込み直す
	UFUNCTION(CallInEditor, Category="Tool|GravityCalculator")
	void LoadCurrentValuesToCalculator();
	
	// 計算ツールの入力で、MaxSpeed / MaxForce / Mass と GravityZ を書き換える
	UFUNCTION(CallInEditor, Category="Tool|GravityCalculator")
	void ApplyGravityFromCalculator();
	
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
private:
	// 計算ツールの入力から、坂で止まる重力を計算する
	float CalcGravityZ() const;
	
	// GravityZ を書き換えて、保存が必要な状態にし、詳細パネルの表示を更新する
	void SetGravityZFromTool(float NewGravityZ);
};
