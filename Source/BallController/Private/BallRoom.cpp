// Author: Claude（Claude が実装。README.md「実装者の記録」参照）
// 床と4枚の壁（と天井）を数値から自動で組み立てる部屋。

#include "BallRoom.h"

#include "BallDoor.h"

#include "Components/DynamicMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "UDynamicMesh.h"
#include "GeometryScript/MeshBooleanFunctions.h"
#include "GeometryScript/MeshPrimitiveFunctions.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName ColorParamName(TEXT("Color"));

	// エンジン標準の Cube は 100cm 角で中心が原点
	constexpr float CubeSize = 100.f;

	UInstancedStaticMeshComponent* MakeBoxes(AActor* Owner, USceneComponent* Parent, const TCHAR* Name, UStaticMesh* Mesh)
	{
		UInstancedStaticMeshComponent* ISM = Owner->CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		ISM->SetupAttachment(Parent);
		ISM->SetMobility(EComponentMobility::Static);
		ISM->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		if (Mesh)
		{
			ISM->SetStaticMesh(Mesh);
		}
		return ISM;
	}
}

ABallRoom::ABallRoom()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Static);
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Cube = CubeMesh.Succeeded() ? CubeMesh.Object : nullptr;

	FloorBoxes = MakeBoxes(this, Root, TEXT("FloorBoxes"), Cube);
	WallBoxes = MakeBoxes(this, Root, TEXT("WallBoxes"), Cube);

	FloorMesh = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("FloorMesh"));
	FloorMesh->SetupAttachment(Root);
	FloorMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BasicMat.Succeeded())
	{
		BaseMaterial = BasicMat.Object;
	}
}

void ABallRoom::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Rebuild();
}

void ABallRoom::Rebuild()
{
	FloorBoxes->ClearInstances();
	WallBoxes->ClearInstances();

	// 坂の部屋などは、部屋の中心をアクターの位置からずらす
	FloorBoxes->SetRelativeLocation(GetCenterOffset());
	WallBoxes->SetRelativeLocation(GetCenterOffset());
	FloorMesh->SetRelativeLocation(GetCenterOffset());
	FloorMesh->GetDynamicMesh()->Reset();

	// 床の穴があるときは、床を FloorMesh に作ってから穴をくり抜く
	bFloorToMesh = bFloor && FloorHoles.Num() > 0;
	if (bFloor)
	{
		BuildFloor();
	}
	if (bFloorToMesh)
	{
		CutFloorHoles();
	}
	// 当たり判定はメッシュの形そのもの（穴のふちも正確に当たる）
	FloorMesh->SetComplexAsSimpleCollisionEnabled(true, true);
	if (bCeiling)
	{
		BuildCeiling();
	}

	if (bWallPlusX)  { BuildWall(ERoomSide::PlusX); }
	if (bWallMinusX) { BuildWall(ERoomSide::MinusX); }
	if (bWallPlusY)  { BuildWall(ERoomSide::PlusY); }
	if (bWallMinusY) { BuildWall(ERoomSide::MinusY); }

	ApplyColor(FloorBoxes, FloorColor, FloorMaterial);
	ApplyColor(WallBoxes, WallColor, WallMaterial);
	FloorMesh->SetMaterial(0, FloorBoxes->GetMaterial(0));

	FitDoors();
}

void ABallRoom::BuildFloor()
{
	{
		// 壁がある面は壁の厚さの半分だけ縮めて、壁の内側に収める（壁が床の下まで伸びて縁を覆う）。
		// 壁をオフにした面は縮めずに、境界線まで伸ばす
		const float HalfT = WallThickness * 0.5f;
		const float X0 = -RoomSize.X * 0.5f + (bWallMinusX ? HalfT : 0.f);
		const float X1 =  RoomSize.X * 0.5f - (bWallPlusX  ? HalfT : 0.f);
		const float Y0 = -RoomSize.Y * 0.5f + (bWallMinusY ? HalfT : 0.f);
		const float Y1 =  RoomSize.Y * 0.5f - (bWallPlusY  ? HalfT : 0.f);
		if (X1 > X0 && Y1 > Y0)
		{
			AddFloorBox(FVector((X0 + X1) * 0.5f, (Y0 + Y1) * 0.5f, -FloorThickness * 0.5f),
				FVector(X1 - X0, Y1 - Y0, FloorThickness));
		}
	}
}

void ABallRoom::AddFloorBox(const FVector& Center, const FVector& Size, const FQuat& Rotation)
{
	if (!bFloorToMesh)
	{
		AddBox(FloorBoxes, Center, Size, Rotation);
		return;
	}
	UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendBox(FloorMesh->GetDynamicMesh(), FGeometryScriptPrimitiveOptions(),
		FTransform(Rotation, Center), Size.X, Size.Y, Size.Z, 0, 0, 0, EGeometryScriptPrimitiveOriginMode::Center);
}

void ABallRoom::CutFloorHoles()
{
	if (!HoleToolMesh)
	{
		HoleToolMesh = NewObject<UDynamicMesh>(this);
	}
	UDynamicMesh* Floor = FloorMesh->GetDynamicMesh();
	const FVector2D Center2D(GetCenterOffset());
	const int32 Segs = FMath::Clamp(HoleCornerSegments, 1, 32);

	for (const FRoomFloorHole& Hole : FloorHoles)
	{
		const float W = FMath::Max(Hole.Size.X, 1.f);
		const float D = FMath::Max(Hole.Size.Y, 1.f);
		const float R = FMath::Clamp(Hole.CornerRadius, 0.f, FMath::Min(W, D) * 0.5f);
		// 部屋の中心を原点にした位置
		const FVector2D C = Hole.Position - Center2D;

		// 真上から見た角丸の四角（反時計回り）。四隅の丸の中心を回って点を並べる
		TArray<FVector2D> Poly;
		const FVector2D Corners[4] = {
			FVector2D( W * 0.5f - R,  D * 0.5f - R),
			FVector2D(-W * 0.5f + R,  D * 0.5f - R),
			FVector2D(-W * 0.5f + R, -D * 0.5f + R),
			FVector2D( W * 0.5f - R, -D * 0.5f + R),
		};
		for (int32 c = 0; c < 4; ++c)
		{
			const int32 N = R > 0.5f ? Segs : 0;
			for (int32 i = 0; i <= N; ++i)
			{
				const float A = FMath::DegreesToRadians(90.f * c + (N > 0 ? 90.f * i / N : 45.f));
				const FVector2D P = Corners[c] + (R > 0.5f ? FVector2D(FMath::Cos(A), FMath::Sin(A)) * R
					: FVector2D(FMath::Sign(FMath::Cos(A)) * W * 0.5f, FMath::Sign(FMath::Sin(A)) * D * 0.5f) - Corners[c]);
				// 直線の辺の長さが 0 のとき（半径 = 辺の半分）は、同じ点が続くので飛ばす
				if (Poly.Num() == 0 || FVector2D::Distance(Poly.Last(), P) > 0.01f)
				{
					Poly.Add(P);
				}
			}
		}
		if (Poly.Num() > 2 && FVector2D::Distance(Poly.Last(), Poly[0]) <= 0.01f)
		{
			Poly.Pop();
		}

		// 床を上下に突き抜ける高さの柱にする（坂なら穴の範囲の高低差も足す）
		const float Z0 = FMath::Min(GetFloorZ(C.X - W * 0.5f), GetFloorZ(C.X + W * 0.5f));
		const float Z1 = FMath::Max(GetFloorZ(C.X - W * 0.5f), GetFloorZ(C.X + W * 0.5f));
		const float Bottom = Z0 - FloorThickness * 3.f - 100.f;
		const float Height = (Z1 + 100.f) - Bottom;

		HoleToolMesh->Reset();
		UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendSimpleExtrudePolygon(HoleToolMesh, FGeometryScriptPrimitiveOptions(),
			FTransform(FVector(C.X, C.Y, Bottom)), Poly, Height, 0, true, EGeometryScriptPrimitiveOriginMode::Base);
		UGeometryScriptLibrary_MeshBooleanFunctions::ApplyMeshBoolean(Floor, FTransform::Identity, HoleToolMesh, FTransform::Identity,
			EGeometryScriptBooleanOperation::Subtract, FGeometryScriptMeshBooleanOptions());
	}
}

void ABallRoom::BuildCeiling()
{
	// 壁の外側まで覆う
	AddBox(FloorBoxes, FVector(0.f, 0.f, WallHeight + CeilingThickness * 0.5f),
		FVector(RoomSize.X + WallThickness, RoomSize.Y + WallThickness, CeilingThickness));
}

void ABallRoom::BuildWall(ERoomSide Side)
{
	if (WallHeight <= 0.f)
	{
		return;
	}

	// ±Y の壁は角まで含めて長く、±X の壁はその間に収める。
	// 相手の面の壁がオフなら、角を作らずに境界線で止める
	const bool bAlongX = (Side == ERoomSide::PlusY || Side == ERoomSide::MinusY);
	const float HalfT = WallThickness * 0.5f;
	float UMin, UMax;
	if (bAlongX)
	{
		UMin = -RoomSize.X * 0.5f - (bWallMinusX ? HalfT : 0.f);
		UMax =  RoomSize.X * 0.5f + (bWallPlusX  ? HalfT : 0.f);
	}
	else
	{
		UMin = -RoomSize.Y * 0.5f + (bWallMinusY ? HalfT : 0.f);
		UMax =  RoomSize.Y * 0.5f - (bWallPlusY  ? HalfT : 0.f);
	}
	if (UMax - UMin <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	// 床がある部屋では、壁を床の下面まで伸ばして床の縁を覆う
	// （高さはすべて、その場所の床の上面からの高さ。坂の部屋では区間ごとに床の高さを足す）
	const float WallBottom = bFloor ? -GetWallBottomDepth() : 0.f;

	// 壁の中心線の位置（壁と直交する方向）
	float Normal = 0.f;
	switch (Side)
	{
	case ERoomSide::PlusX:  Normal =  RoomSize.X * 0.5f; break;
	case ERoomSide::MinusX: Normal = -RoomSize.X * 0.5f; break;
	case ERoomSide::PlusY:  Normal =  RoomSize.Y * 0.5f; break;
	case ERoomSide::MinusY: Normal = -RoomSize.Y * 0.5f; break;
	}

	// この面の穴を、壁に沿った区間 [U0,U1] と高さの区間 [Z0,Z1] にする
	struct FHole { float U0, U1, Z0, Z1; };
	TArray<FHole> Holes;
	TArray<float> Cuts = { UMin, UMax };
	for (const FRoomOpening& O : Openings)
	{
		if (O.Side != Side || O.Width <= 0.f || (O.bUseHeight && O.Height <= 0.f))
		{
			continue;
		}
		FHole H;
		H.U0 = FMath::Clamp(O.Offset - O.Width * 0.5f, UMin, UMax);
		H.U1 = FMath::Clamp(O.Offset + O.Width * 0.5f, UMin, UMax);
		H.Z0 = FMath::Clamp(O.Bottom, 0.f, WallHeight);
		// 扉のある床ぎわの穴は、穴の下の壁も作らない（沈んだ扉が入る場所）
		if (O.Door && O.Bottom <= 0.f)
		{
			H.Z0 = WallBottom;
		}
		// 高さを使わない穴は、壁の上端まで抜く
		H.Z1 = O.bUseHeight ? FMath::Clamp(O.Bottom + O.Height, 0.f, WallHeight) : WallHeight;
		if (H.U1 - H.U0 > KINDA_SMALL_NUMBER && H.Z1 - H.Z0 > KINDA_SMALL_NUMBER)
		{
			Holes.Add(H);
			Cuts.Add(H.U0);
			Cuts.Add(H.U1);
		}
	}

	Cuts.Sort();

	// 穴の端で壁を縦に区切り、区間ごとに穴のない高さだけ箱を置く
	for (int32 i = 0; i + 1 < Cuts.Num(); ++i)
	{
		const float U0 = Cuts[i];
		const float U1 = Cuts[i + 1];
		if (U1 - U0 <= KINDA_SMALL_NUMBER)
		{
			continue;
		}
		const float UMid = (U0 + U1) * 0.5f;

		// この区間にかかる穴の高さ区間を集めて、下から順に並べる
		TArray<TPair<float, float>> ZHoles;
		for (const FHole& H : Holes)
		{
			if (H.U0 <= UMid && UMid <= H.U1)
			{
				ZHoles.Add({ H.Z0, H.Z1 });
			}
		}
		ZHoles.Sort([](const TPair<float, float>& A, const TPair<float, float>& B) { return A.Key < B.Key; });

		// 穴の高さは床の上面（0）から測るので、穴の下には床の厚さぶんの壁が残る
		float Z = WallBottom;
		auto EmitSolid = [&](float ZFrom, float ZTo)
		{
			if (ZTo - ZFrom > KINDA_SMALL_NUMBER)
			{
				AddWallPiece(bAlongX, Normal, U0, U1, ZFrom, ZTo);
			}
		};
		for (const TPair<float, float>& ZH : ZHoles)
		{
			EmitSolid(Z, ZH.Key);
			Z = FMath::Max(Z, ZH.Value);
		}
		EmitSolid(Z, WallHeight);
	}
}

void ABallRoom::AddWallPiece(bool bAlongX, float Normal, float U0, float U1, float ZFrom, float ZTo)
{
	// その場所の床の上面の高さを足す（平らな部屋では 0）
	const float UMid = (U0 + U1) * 0.5f;
	const float ZMid = GetFloorZ(bAlongX ? UMid : Normal) + (ZFrom + ZTo) * 0.5f;
	const float Height = ZTo - ZFrom;
	const float Width = U1 - U0;
	if (bAlongX)
	{
		AddBox(WallBoxes, FVector(UMid, Normal, ZMid), FVector(Width, WallThickness, Height));
	}
	else
	{
		AddBox(WallBoxes, FVector(Normal, UMid, ZMid), FVector(WallThickness, Width, Height));
	}
}

void ABallRoom::FitDoors()
{
	UWorld* World = GetWorld();
	if (!World || World->IsGameWorld())
	{
		// ゲーム中は保存済みの位置をそのまま使う
		return;
	}

	const FTransform RoomTM = GetActorTransform();
	for (const FRoomOpening& O : Openings)
	{
		ABallDoor* Door = O.Door;
		if (!Door || O.Width <= 0.f)
		{
			continue;
		}

		const float Height = O.bUseHeight ? O.Height : FMath::Max(WallHeight - O.Bottom, 0.f);
		if (Height <= 0.f)
		{
			continue;
		}

		// 扉の足元（穴の下端の中心、壁の中心線上）と向き（扉のローカル X = 壁の厚さ方向）
		FVector Local = FVector::ZeroVector;
		float Yaw = 0.f;
		switch (O.Side)
		{
		case ERoomSide::PlusX:  Local = FVector( RoomSize.X * 0.5f, O.Offset, O.Bottom); Yaw = 0.f;  break;
		case ERoomSide::MinusX: Local = FVector(-RoomSize.X * 0.5f, O.Offset, O.Bottom); Yaw = 0.f;  break;
		case ERoomSide::PlusY:  Local = FVector(O.Offset,  RoomSize.Y * 0.5f, O.Bottom); Yaw = 90.f; break;
		case ERoomSide::MinusY: Local = FVector(O.Offset, -RoomSize.Y * 0.5f, O.Bottom); Yaw = 90.f; break;
		}

		// 穴の下端は、その場所の床の上面から測る。部屋の中心のずれも足す
		Local.Z += GetFloorZ(Local.X);
		Local += GetCenterOffset();

		const FVector WorldLoc = RoomTM.TransformPosition(Local);
		const FQuat WorldRot = RoomTM.GetRotation() * FRotator(0.f, Yaw, 0.f).Quaternion();
		Door->SetActorTransform(FTransform(WorldRot, WorldLoc, FVector::OneVector));
		Door->FitSize(O.Width, Height, WallThickness);
#if WITH_EDITOR
		Door->MarkPackageDirty();
#endif
	}
}

void ABallRoom::AddBox(UInstancedStaticMeshComponent* ISM, const FVector& Center, const FVector& Size, const FQuat& Rotation)
{
	const FTransform T(Rotation, Center, Size / CubeSize);
	ISM->AddInstance(T, /*bWorldSpace=*/false);
}

void ABallRoom::ApplyColor(UInstancedStaticMeshComponent* ISM, const FLinearColor& Color, TObjectPtr<UMaterialInstanceDynamic>& MID)
{
	if (!BaseMaterial)
	{
		return;
	}
	// 元のマテリアルが変わったときだけ作り直し、それ以外は色だけ変える
	// （ドラッグ中は OnConstruction が毎フレーム呼ばれるため）
	if (!MID || MID->Parent != BaseMaterial)
	{
		MID = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	}
	MID->SetVectorParameterValue(ColorParamName, Color);
	if (ISM->GetMaterial(0) != MID)
	{
		ISM->SetMaterial(0, MID);
	}
}
