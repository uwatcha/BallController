// Author: Claude（Claude が実装。README.md「実装者の記録」参照）
// MyPrimaryDataAsset の、坂で止まる重力の計算ツール (Tool|GravityCalculator)。

#include "MyPrimaryDataAsset.h"

#include "PhysicsEngine/PhysicsSettings.h"

void UMyPrimaryDataAsset::PostLoad()
{
	Super::PostLoad();
	// 開いた時点の本来の値を、計算ツールの入力欄に入れておく
	LoadCurrentValuesToCalculator();
}

void UMyPrimaryDataAsset::LoadCurrentValuesToCalculator()
{
	CalcMaxSpeed = MaxSpeed;
	CalcMaxForce = MaxForce;
	CalcMass = Mass;
	CalcGravityZInput = GravityZ;
	CalcResultGravityZ = CalcGravityZ();
}

float UMyPrimaryDataAsset::CalcGravityZ() const
{
	// 鉄球に加わる水平の加速度 (cm/s²)。Move() では「MaxForce × ForceVectorScale」を AddForce に渡している
	const float Push = CalcMaxForce * ForceVectorScale;
	const float Accel = UseAcceleration ? Push : Push / FMath::Max(CalcMass, 1.f);
	
	// 水平に押す力の坂に沿った成分 (Accel × cos θ) と、重力の坂を下る成分 (g × sin θ) が釣り合う重力
	// → g = Accel / tan θ
	const float Tan = FMath::Tan(FMath::DegreesToRadians(CalcSlopeAngle));
	if (Tan <= KINDA_SMALL_NUMBER)
	{
		return GravityZ;
	}
	return -(Accel / Tan) * CalcGravityRate;
}

void UMyPrimaryDataAsset::ApplyGravityFromCalculator()
{
	Modify();
	MaxSpeed = CalcMaxSpeed;
	MaxForce = CalcMaxForce;
	Mass = CalcMass;
	CalcResultGravityZ = CalcGravityZ();
	SetGravityZFromTool(CalcResultGravityZ);
}

void UMyPrimaryDataAsset::ApplyGravityZInput()
{
	SetGravityZFromTool(CalcGravityZInput);
}

void UMyPrimaryDataAsset::ResetGravityZ()
{
	SetGravityZFromTool(UPhysicsSettings::Get()->DefaultGravityZ);
}

void UMyPrimaryDataAsset::SetGravityZFromTool(float NewGravityZ)
{
	Modify();
	GravityZ = NewGravityZ;
	// 直接入力の欄も今の値に合わせる
	CalcGravityZInput = GravityZ;
	MarkPackageDirty();
#if WITH_EDITOR
	// 詳細パネルの表示を更新する
	PostEditChange();
#endif
}

#if WITH_EDITOR
void UMyPrimaryDataAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	// 入力を変えるたびに、計算結果の表示を更新する
	CalcResultGravityZ = CalcGravityZ();
}
#endif
