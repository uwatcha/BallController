// Author: Claude（Claude が実装。README.md「実装者の記録」参照）
// 鉄球や鉄箱（物理シミュレーション中のアクター）が乗ると押されるスイッチ。

#include "BallSwitch.h"

#include "BallLog.h"
#include "EngineUtils.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// エンジン標準の BasicShapeMaterial には "Color" というベクトルパラメータがある
	const FName ColorParamName(TEXT("Color"));
}

ABallSwitch::ABallSwitch()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// 見た目: 100x100x4cm の板（床から 2cm 出る）。当たり判定は付けない
	PlateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlateMesh"));
	PlateMesh->SetupAttachment(Root);
	PlateMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlateMesh->SetGenerateOverlapEvents(false);
	PlateMesh->SetRelativeScale3D(FVector(1.f, 1.f, 0.04f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		PlateMesh->SetStaticMesh(CubeMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BasicMat.Succeeded())
	{
		PlateMesh->SetMaterial(0, BasicMat.Object);
	}

	// 押される範囲: 板の上に高さ 60cm の箱。中身の判定は Tick で問い合わせるので、箱自体の当たり判定はなし
	PressArea = CreateDefaultSubobject<UBoxComponent>(TEXT("PressArea"));
	PressArea->SetupAttachment(Root);
	PressArea->SetBoxExtent(FVector(50.f, 50.f, 30.f));
	PressArea->SetRelativeLocation(FVector(0.f, 0.f, 30.f));
	PressArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PressArea->SetGenerateOverlapEvents(false);
	PressArea->SetHiddenInGame(true);
}

void ABallSwitch::BeginPlay()
{
	Super::BeginPlay();

	PlateMaterial = PlateMesh->CreateAndSetMaterialInstanceDynamic(0);

	// グループのメンバーを集める（同じ GroupName のスイッチ）
	GroupMembers.Reset();
	if (GroupName.IsNone())
	{
		GroupMembers.Add(this);
	}
	else
	{
		for (TActorIterator<ABallSwitch> It(GetWorld()); It; ++It)
		{
			if (It->GroupName == GroupName)
			{
				GroupMembers.Add(*It);
			}
		}
	}

	UpdateColor();
}

void ABallSwitch::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Once で一度押されたら、もう調べる必要はない
	if (Mode == EBallSwitchMode::Once && bPressed)
	{
		SetActorTickEnabled(false);
		return;
	}

	const bool bNow = CheckPressedNow();
	if (bNow == bPressed)
	{
		return;
	}

	bPressed = bNow;
	SB_LOG(LogTemp, Log, TEXT("%s pressed=%d (Group=%s)"), *GetName(), bPressed, *GroupName.ToString());
	RefreshGroup();
}

bool ABallSwitch::CheckPressedNow() const
{
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(BallSwitch), false, this);

	GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		PressArea->GetComponentLocation(),
		PressArea->GetComponentQuat(),
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllDynamicObjects),
		FCollisionShape::MakeBox(PressArea->GetScaledBoxExtent()),
		Params);

	for (const FOverlapResult& Result : Overlaps)
	{
		const UPrimitiveComponent* Comp = Result.GetComponent();
		// 押せるのは物理シミュレーション中のもの（鉄球・鉄箱）だけ
		if (Comp && Comp->IsSimulatingPhysics() && Comp->GetMass() >= MinMassKg)
		{
			return true;
		}
	}
	return false;
}

void ABallSwitch::RefreshGroup()
{
	bool bAllPressed = true;
	for (const TWeakObjectPtr<ABallSwitch>& Member : GroupMembers)
	{
		if (Member.IsValid() && !Member->bPressed)
		{
			bAllPressed = false;
			break;
		}
	}

	for (const TWeakObjectPtr<ABallSwitch>& Member : GroupMembers)
	{
		if (Member.IsValid())
		{
			Member->ApplyGroupState(bAllPressed);
		}
	}
}

void ABallSwitch::ApplyGroupState(bool bGroupAllPressed)
{
	const bool bWasActive = bActive;
	bActive = bGroupAllPressed;
	UpdateColor();

	if (bActive != bWasActive)
	{
		SB_LOG(LogTemp, Log, TEXT("%s active=%d"), *GetName(), bActive);
		OnActiveChanged.Broadcast(this, bActive);
	}
}

void ABallSwitch::UpdateColor()
{
	if (PlateMaterial == nullptr)
	{
		return;
	}

	const FLinearColor& Color = bActive ? ColorActive : (bPressed ? ColorWaitingGroup : ColorReleased);
	PlateMaterial->SetVectorParameterValue(ColorParamName, Color);
}
