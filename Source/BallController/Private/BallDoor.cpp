// Author: Claude（Claude が実装。README.md「実装者の記録」参照）
// スイッチ（ABallSwitch）と連動し、条件を満たすと下に沈んで通れるようになる扉。

#include "BallDoor.h"

#include "BallLog.h"
#include "BallSwitch.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// エンジン標準の BasicShapeMaterial には "Color" というベクトルパラメータがある
	const FName ColorParamName(TEXT("Color"));
}

ABallDoor::ABallDoor()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// 本体: 幅 400 x 厚さ 50 x 高さ 300cm の板。Root（足元）から上に立つ
	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(Root);
	DoorMesh->SetMobility(EComponentMobility::Movable);
	DoorMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	DoorMesh->SetRelativeScale3D(FVector(0.5f, 4.f, 3.f));
	DoorMesh->SetRelativeLocation(FVector(0.f, 0.f, 150.f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		DoorMesh->SetStaticMesh(CubeMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BasicMat.Succeeded())
	{
		BaseMaterial = BasicMat.Object;
		DoorMesh->SetMaterial(0, BasicMat.Object);
	}
}

void ABallDoor::BeginPlay()
{
	Super::BeginPlay();

	ApplyColor();

	ClosedLocation = DoorMesh->GetComponentLocation();
	// 0 なら扉の高さぶん沈める（少し余分に沈めて、床から出っ張らないようにする）
	ActualOpenDistance = OpenDistance > 0.f ? OpenDistance : DoorMesh->Bounds.BoxExtent.Z * 2.f + 2.f;

	for (ABallSwitch* Switch : Switches)
	{
		if (Switch)
		{
			Switch->OnActiveChanged.AddDynamic(this, &ABallDoor::HandleSwitchChanged);
		}
	}

	if (Switches.Num() == 0)
	{
		SB_LOG(LogTemp, Warning, TEXT("%s: Switches が空なので開きません"), *GetName());
	}

	EvaluateSwitches();
}

void ABallDoor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyColor();
}

void ABallDoor::ApplyColor()
{
	// メッシュのマテリアル欄ではなく BaseMaterial から作る。
	// （「インスタンス変更をBPに適用」で欄が WorldGridMaterial になっても、ここで元に戻る）
	if (DoorMaterial == nullptr || DoorMaterial->Parent != BaseMaterial)
	{
		DoorMaterial = BaseMaterial ? UMaterialInstanceDynamic::Create(BaseMaterial, this) : nullptr;
	}
	if (DoorMaterial && DoorMesh->GetMaterial(0) != DoorMaterial)
	{
		DoorMesh->SetMaterial(0, DoorMaterial);
	}
	if (DoorMaterial)
	{
		DoorMaterial->SetVectorParameterValue(ColorParamName, DoorColor);
	}
}

void ABallDoor::HandleSwitchChanged(ABallSwitch* Switch, bool bActive)
{
	EvaluateSwitches();
}

void ABallDoor::EvaluateSwitches()
{
	int32 ValidCount = 0;
	int32 ActiveCount = 0;
	for (const ABallSwitch* Switch : Switches)
	{
		if (Switch)
		{
			++ValidCount;
			if (Switch->IsActive())
			{
				++ActiveCount;
			}
		}
	}

	const bool bShouldOpen = ValidCount > 0 && (bRequireAllSwitches ? ActiveCount == ValidCount : ActiveCount > 0);
	if (bShouldOpen != bOpenRequested)
	{
		bOpenRequested = bShouldOpen;
		OpenRequestedTime = 0.f;
		SB_LOG(LogTemp, Log, TEXT("%s open=%d"), *GetName(), bOpenRequested);
	}
}

void ABallDoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	float Target = 0.f;
	float Speed = CloseSpeed;
	if (bOpenRequested)
	{
		OpenRequestedTime += DeltaSeconds;
		if (OpenRequestedTime >= OpenDelay)
		{
			Target = ActualOpenDistance;
			Speed = OpenSpeed;
		}
		else
		{
			// 待ち時間中は今の位置で止めておく
			Target = CurrentOffset;
		}
	}

	if (FMath::IsNearlyEqual(CurrentOffset, Target))
	{
		return;
	}

	CurrentOffset = FMath::FInterpConstantTo(CurrentOffset, Target, DeltaSeconds, Speed);
	// 物理の当たり判定も同じ位置へすぐ移す
	DoorMesh->SetWorldLocation(ClosedLocation - FVector::UpVector * CurrentOffset, false, nullptr, ETeleportType::TeleportPhysics);

	// 開ききったら当たり判定を切り、確実に通れるようにする。閉じ始めたら戻す
	const bool bFullyOpen = CurrentOffset >= ActualOpenDistance - 1.f;
	const ECollisionEnabled::Type WantCollision = bFullyOpen ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics;
	if (DoorMesh->GetCollisionEnabled() != WantCollision)
	{
		DoorMesh->SetCollisionEnabled(WantCollision);
		SB_LOG(LogTemp, Log, TEXT("%s collision=%s"), *GetName(), bFullyOpen ? TEXT("off") : TEXT("on"));
	}
}
