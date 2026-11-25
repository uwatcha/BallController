// Author: Claude（Claude が実装。README.md「実装者の記録」参照）
// 当たった鉄球を、中心から外向きに弾き飛ばす円柱のバンパー。

#include "BallBumper.h"

#include "BallLog.h"
#include "MyPrimaryDataAsset.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ABallBumper::ABallBumper()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	// 当たったときに OnComponentHit を呼ぶ
	Mesh->SetNotifyRigidBodyCollision(true);

	// エンジン標準の Cylinder は直径 100cm・高さ 100cm で中心が原点
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CylinderMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BasicMat.Succeeded())
	{
		BaseMaterial = BasicMat.Object;
	}
}

void ABallBumper::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	Mesh->SetRelativeScale3D(FVector(Diameter, Diameter, Height) / 100.f);
	Mesh->SetRelativeLocation(FVector(0.f, 0.f, Height * 0.5f));

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

void ABallBumper::BeginPlay()
{
	Super::BeginPlay();

	Mesh->OnComponentHit.AddDynamic(this, &ABallBumper::HandleHit);

	if (!TuningData)
	{
		SB_LOG(LogTemp, Warning, TEXT("%s: TuningData が空なので弾きません"), *GetName());
	}
}

void ABallBumper::HandleHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!TuningData || !OtherComp || !OtherComp->IsSimulatingPhysics())
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastBounceTime < TuningData->BumperCooldown)
	{
		return;
	}
	LastBounceTime = Now;

	// バンパーの中心から相手への水平な向き
	FVector Dir = OtherComp->GetComponentLocation() - GetActorLocation();
	Dir.Z = 0.f;
	if (!Dir.Normalize())
	{
		Dir = -Hit.ImpactNormal;
		Dir.Z = 0.f;
		Dir.Normalize();
	}

	// 水平方向の速さを BumperBounceSpeed に置き換える（上下の速さはそのまま）
	const FVector Vel = OtherComp->GetPhysicsLinearVelocity();
	OtherComp->SetPhysicsLinearVelocity(Dir * TuningData->BumperBounceSpeed + FVector(0.f, 0.f, Vel.Z));
}
