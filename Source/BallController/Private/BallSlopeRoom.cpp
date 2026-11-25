// Author: Claude（Claude が実装。README.md「実装者の記録」参照）
// 下り坂／上り坂の部屋。

#include "BallSlopeRoom.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "ProceduralMeshComponent.h"

ABallSlopeRoom::ABallSlopeRoom()
{
	SideWalls = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("SideWalls"));
	SideWalls->SetupAttachment(Root);
	// 当たり判定は、壁1つごとの凸形状（下の AddCollisionConvexMesh）で作る
	SideWalls->bUseComplexAsSimpleCollision = false;
	SideWalls->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
}

void ABallSlopeRoom::OnConstruction(const FTransform& Transform)
{
	EndPoint = FVector(RoomSize.X, 0.f, RoomSize.X * GetSlopeTan());

	WallVerts.Reset();
	WallTris.Reset();
	WallNormals.Reset();
	WallUVs.Reset();
	SideWalls->ClearAllMeshSections();
	SideWalls->ClearCollisionConvexMeshes();

	// ここで壁が組み立てられ、斜めの部分は AddWallPiece から WallVerts などにたまる
	Super::OnConstruction(Transform);

	SideWalls->SetRelativeLocation(GetCenterOffset());
	if (WallVerts.Num() > 0)
	{
		SideWalls->CreateMeshSection_LinearColor(0, WallVerts, WallTris, WallNormals, WallUVs,
			TArray<FLinearColor>(), TArray<FProcMeshTangent>(), /*bCreateCollision=*/false);
	}
	// 色は箱の壁と同じマテリアルを使う
	SideWalls->SetMaterial(0, WallBoxes->GetMaterial(0));
}

float ABallSlopeRoom::GetSlopeTan() const
{
	const float Tan = FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(SlopeAngle, 0.f, 60.f)));
	return Direction == EBallSlopeDirection::Up ? Tan : -Tan;
}

float ABallSlopeRoom::SlopeZ(float LocalX) const
{
	// 始点（X = -RoomSize.X / 2）で高さ 0
	return (LocalX + RoomSize.X * 0.5f) * GetSlopeTan();
}

float ABallSlopeRoom::GetFloorZ(float LocalX) const
{
	// 坂の外（角の壁など）は、端の高さのまま平らにする
	return SlopeZ(FMath::Clamp(LocalX, -RoomSize.X * 0.5f, RoomSize.X * 0.5f));
}

float ABallSlopeRoom::GetWallBottomDepth() const
{
	// 斜めの床は縦に測ると厚く、両端の壁の厚みの中でも高さが変わるので、そのぶん壁を下まで伸ばして床の縁を覆う
	const float AbsTan = FMath::Abs(GetSlopeTan());
	const float SlabDepth = FloorThickness * FMath::Sqrt(1.f + AbsTan * AbsTan);
	return SlabDepth + WallThickness * 0.5f * AbsTan;
}

FVector ABallSlopeRoom::GetCenterOffset() const
{
	// アクターの位置を始点にする
	return FVector(RoomSize.X * 0.5f, 0.f, 0.f);
}

void ABallSlopeRoom::AddSlopedSlab(float X0, float X1, float Y0, float Y1, float TopZOffset, float Thickness)
{
	if (X1 - X0 <= KINDA_SMALL_NUMBER || Y1 - Y0 <= KINDA_SMALL_NUMBER || Thickness <= 0.f)
	{
		return;
	}
	const float Tan = GetSlopeTan();
	const FQuat Rot = FRotator(FMath::RadiansToDegrees(FMath::Atan(Tan)), 0.f, 0.f).Quaternion();
	const FVector Up = Rot.RotateVector(FVector::UpVector);
	const float Length = (X1 - X0) * FMath::Sqrt(1.f + Tan * Tan);
	const FVector TopMid((X0 + X1) * 0.5f, (Y0 + Y1) * 0.5f, SlopeZ((X0 + X1) * 0.5f) + TopZOffset);
	AddBox(FloorBoxes, TopMid - Up * (Thickness * 0.5f), FVector(Length, Y1 - Y0, Thickness), Rot);
}

void ABallSlopeRoom::BuildFloor()
{
	// 床の範囲の決め方は ABallRoom::BuildFloor と同じ（壁がある面は壁の厚さの半分だけ縮める）
	const float HalfT = WallThickness * 0.5f;
	const float X0 = -RoomSize.X * 0.5f + (bWallMinusX ? HalfT : 0.f);
	const float X1 =  RoomSize.X * 0.5f - (bWallPlusX  ? HalfT : 0.f);
	const float Y0 = -RoomSize.Y * 0.5f + (bWallMinusY ? HalfT : 0.f);
	const float Y1 =  RoomSize.Y * 0.5f - (bWallPlusY  ? HalfT : 0.f);
	AddSlopedSlab(X0, X1, Y0, Y1, 0.f, FloorThickness);
}

void ABallSlopeRoom::BuildCeiling()
{
	// 床と平行に、壁の高さの位置に置く（壁の外側まで覆う）
	const float HalfT = WallThickness * 0.5f;
	AddSlopedSlab(-RoomSize.X * 0.5f - HalfT, RoomSize.X * 0.5f + HalfT,
		-RoomSize.Y * 0.5f - HalfT, RoomSize.Y * 0.5f + HalfT,
		WallHeight + CeilingThickness, CeilingThickness);
}

void ABallSlopeRoom::AddWallPiece(bool bAlongX, float Normal, float U0, float U1, float ZFrom, float ZTo)
{
	// ±X の壁と、平らな部屋は今までどおりの箱
	if (!bAlongX || FMath::IsNearlyZero(GetSlopeTan()))
	{
		Super::AddWallPiece(bAlongX, Normal, U0, U1, ZFrom, ZTo);
		return;
	}

	// 坂の外（角の部分）は、端の高さの平らな箱
	const float S0 = -RoomSize.X * 0.5f;
	const float S1 =  RoomSize.X * 0.5f;
	if (U0 < S0)
	{
		Super::AddWallPiece(bAlongX, Normal, U0, FMath::Min(U1, S0), ZFrom, ZTo);
	}
	if (U1 > S1)
	{
		Super::AddWallPiece(bAlongX, Normal, FMath::Max(U0, S1), U1, ZFrom, ZTo);
	}

	// 坂の上の部分は斜めの壁
	const float A = FMath::Max(U0, S0);
	const float B = FMath::Min(U1, S1);
	if (B - A > KINDA_SMALL_NUMBER)
	{
		AddSlopedWall(A, B, Normal, ZFrom, ZTo);
	}
}

void ABallSlopeRoom::AddSlopedWall(float X0, float X1, float Normal, float ZFrom, float ZTo)
{
	const float Y0 = Normal - WallThickness * 0.5f;
	const float Y1 = Normal + WallThickness * 0.5f;
	const float B0 = SlopeZ(X0) + ZFrom, T0 = SlopeZ(X0) + ZTo;   // 始点側の下端・上端
	const float B1 = SlopeZ(X1) + ZFrom, T1 = SlopeZ(X1) + ZTo;   // 終点側の下端・上端

	// 8つの角
	const FVector P000(X0, Y0, B0), P100(X1, Y0, B1), P010(X0, Y1, B0), P110(X1, Y1, B1);
	const FVector P001(X0, Y0, T0), P101(X1, Y0, T1), P011(X0, Y1, T0), P111(X1, Y1, T1);

	const FVector TopN = FVector(-GetSlopeTan(), 0.f, 1.f).GetSafeNormal();
	AddQuad(P001, P101, P111, P011, TopN);                  // 上
	AddQuad(P000, P010, P110, P100, -TopN);                 // 下
	AddQuad(P000, P100, P101, P001, FVector(0.f, -1.f, 0.f)); // -Y 側
	AddQuad(P010, P011, P111, P110, FVector(0.f,  1.f, 0.f)); // +Y 側
	AddQuad(P000, P001, P011, P010, FVector(-1.f, 0.f, 0.f)); // 始点側
	AddQuad(P100, P110, P111, P101, FVector( 1.f, 0.f, 0.f)); // 終点側

	// 当たり判定（8つの角を囲む凸形状）
	SideWalls->AddCollisionConvexMesh({ P000, P100, P010, P110, P001, P101, P011, P111 });
}

void ABallSlopeRoom::AddQuad(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, const FVector& Normal)
{
	const int32 Base = WallVerts.Num();
	for (const FVector& P : { P0, P1, P2, P3 })
	{
		WallVerts.Add(P);
		WallNormals.Add(Normal);
		// 1m = UV 1 のかんたんな投影
		const FVector2D UV = FMath::Abs(Normal.Z) > 0.5f ? FVector2D(P.X, P.Y)
			: (FMath::Abs(Normal.X) > 0.5f ? FVector2D(P.Y, P.Z) : FVector2D(P.X, P.Z));
		WallUVs.Add(UV / 100.f);
	}
	// 表裏どちらの向きでも見えるように、両回りの三角形を入れる
	WallTris.Append({ Base, Base + 1, Base + 2, Base, Base + 2, Base + 3 });
	WallTris.Append({ Base, Base + 2, Base + 1, Base, Base + 3, Base + 2 });
}
