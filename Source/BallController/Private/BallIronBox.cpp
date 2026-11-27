// Author: Claude（ファイル全体を Claude が実装。README.md「実装者の記録」参照）
// マグネティックフォースで引き寄せられる鉄箱。

#include "BallIronBox.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ABallIronBox::ABallIronBox()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	// 物理で動く物の標準のプロファイル (Object Type = PhysicsBody)
	Mesh->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	Mesh->SetSimulatePhysics(true);

	// エンジン標準の Cube は 100cm 四方で中心が原点
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BasicMat.Succeeded())
	{
		BaseMaterial = BasicMat.Object;
	}
}

void ABallIronBox::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 大きさ: ルートのスケール = アクターのスケール
	Mesh->SetWorldScale3D(Size / 100.f);
	Mesh->SetMassOverrideInKg(NAME_None, MassKg, true);

	// 引き寄せ対象のタグ。名前を変えたときに古いタグが残らないよう、付け直す
	Tags.Remove(LastMagneticTag);
	if (!MagneticTag.IsNone())
	{
		Tags.AddUnique(MagneticTag);
	}
	LastMagneticTag = MagneticTag;

	if (BaseMaterial)
	{
		if (!MeshMaterial || MeshMaterial->Parent != BaseMaterial)
		{
			MeshMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		}
		MeshMaterial->SetVectorParameterValue(TEXT("Color"), Color);
		if (Mesh->GetMaterial(0) != MeshMaterial)
		{
			Mesh->SetMaterial(0, MeshMaterial);
		}
	}
}

void ABallIronBox::BeginPlay()
{
	Super::BeginPlay();

	// 固定: 物理シミュレーションを切ると、当たり判定はそのままで、押しても動かない
	if (bLockUntilMagnet)
	{
		Mesh->SetSimulatePhysics(false);
		bLocked = true;
	}
}

void ABallIronBox::UnlockByMagnet()
{
	if (!bLocked) return;
	bLocked = false;
	Mesh->SetSimulatePhysics(true);
	Mesh->WakeRigidBody();
}
