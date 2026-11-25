// Author: Claude（Claude が実装。README.md「実装者の記録」参照）
// 床と4枚の壁（と天井）を数値から自動で組み立てる部屋。

#include "BallRoom.h"

#include "Components/InstancedStaticMeshComponent.h"
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

	if (bFloor)
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
			AddBox(FloorBoxes, FVector((X0 + X1) * 0.5f, (Y0 + Y1) * 0.5f, -FloorThickness * 0.5f),
				FVector(X1 - X0, Y1 - Y0, FloorThickness));
		}
	}
	if (bCeiling)
	{
		// 壁の外側まで覆う
		AddBox(FloorBoxes, FVector(0.f, 0.f, WallHeight + CeilingThickness * 0.5f),
			FVector(RoomSize.X + WallThickness, RoomSize.Y + WallThickness, CeilingThickness));
	}

	if (bWallPlusX)  { BuildWall(ERoomSide::PlusX); }
	if (bWallMinusX) { BuildWall(ERoomSide::MinusX); }
	if (bWallPlusY)  { BuildWall(ERoomSide::PlusY); }
	if (bWallMinusY) { BuildWall(ERoomSide::MinusY); }

	ApplyColor(FloorBoxes, FloorColor, FloorMaterial);
	ApplyColor(WallBoxes, WallColor, WallMaterial);
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
	const float WallBottom = bFloor ? -FloorThickness : 0.f;

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
			if (ZTo - ZFrom <= KINDA_SMALL_NUMBER)
			{
				return;
			}
			const float ZMid = (ZFrom + ZTo) * 0.5f;
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
		};
		for (const TPair<float, float>& ZH : ZHoles)
		{
			EmitSolid(Z, ZH.Key);
			Z = FMath::Max(Z, ZH.Value);
		}
		EmitSolid(Z, WallHeight);
	}
}

void ABallRoom::AddBox(UInstancedStaticMeshComponent* ISM, const FVector& Center, const FVector& Size)
{
	const FTransform T(FQuat::Identity, Center, Size / CubeSize);
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
