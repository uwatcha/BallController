// Author: Claude（ファイル全体を Claude が実装。README.md「実装者の記録」参照）
// 能力の取得アイテム。

#include "BallAbilityPickup.h"

#include "BallAbilitySubsystem.h"
#include "BallLog.h"
#include "MyPrimaryDataAsset.h"
#include "Blueprint/UserWidget.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ABallAbilityPickup::ABallAbilityPickup()
{
	// 鉄球が触れたかを毎フレーム調べる（BallSwitch と同じく、オーバーラップの設定を不要にするため）
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	ShellMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShellMesh"));
	ShellMesh->SetupAttachment(Root);
	ShellMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShellMesh->SetGenerateOverlapEvents(false);

	IconMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("IconMesh"));
	IconMesh->SetupAttachment(Root);
	IconMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	IconMesh->SetGenerateOverlapEvents(false);
	IconMesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		ShellMesh->SetStaticMesh(SphereMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMesh.Succeeded())
	{
		IconMesh->SetStaticMesh(PlaneMesh.Object);
	}
}

float ABallAbilityPickup::GetDiameter() const
{
	return TuningData ? TuningData->BallDiameter : Diameter;
}

void ABallAbilityPickup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Sphere も Plane も 100cm
	const float D = FMath::Max(GetDiameter(), 1.f);
	ShellMesh->SetRelativeScale3D(FVector(D / 100.f));
	IconMesh->SetRelativeScale3D(FVector(D * IconSizeRate / 100.f));

	if (ShellMaterial) ShellMesh->SetMaterial(0, ShellMaterial);
	if (IconMaterial) IconMesh->SetMaterial(0, IconMaterial);
}

void ABallAbilityPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bPickedUp) return;

	UpdateIconRotation(DeltaTime);

	// 球の中に、プレイヤーが操作している Pawn（鉄球）の当たり判定が入ったら取得
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(AbilityPickup), false, this);
	GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		GetActorLocation(),
		FQuat::Identity,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllDynamicObjects),
		FCollisionShape::MakeSphere(GetDiameter() * 0.5f),
		Params);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		APawn* Pawn = Cast<APawn>(Overlap.GetActor());
		if (Pawn && Pawn->IsPlayerControlled())
		{
			PickUp(Pawn);
			return;
		}
	}
}

void ABallAbilityPickup::UpdateIconRotation(float DeltaTime)
{
	// カメラマネージャーが取れないとき（エディタ上など）は何もしない
	const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!Camera) return;

	SpinAngle = FMath::Fmod(SpinAngle + IconSpinSpeed * DeltaTime, 360.f);

	FQuat Base = FQuat::Identity;
	FVector SpinAxis = FVector::UpVector;
	if (bFaceCamera)
	{
		const FRotationMatrix CamMatrix(Camera->GetCameraRotation());
		const FVector CamForward = CamMatrix.GetUnitAxis(EAxis::X);
		const FVector CamUp = CamMatrix.GetUnitAxis(EAxis::Z);
		// 板の +Z をカメラへ、板の Y を画面の上へ
		Base = FRotationMatrix::MakeFromZY(-CamForward, CamUp).ToQuat();
		SpinAxis = CamUp;
	}

	const FQuat Image(FVector::UpVector, FMath::DegreesToRadians(IconImageAngle));
	const FQuat Spin(SpinAxis, FMath::DegreesToRadians(SpinAngle));
	IconMesh->SetWorldRotation(Spin * Base * Image);
}

void ABallAbilityPickup::PickUp(APawn* Picker)
{
	bPickedUp = true;

	if (UBallAbilitySubsystem* Abilities = GetWorld()->GetSubsystem<UBallAbilitySubsystem>())
	{
		Abilities->UnlockAbility(Ability);
	}

	// 見た目を消す。Blueprint のイベントやパネルのタイマーが動けるよう、アクター自体は残す
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);

	if (PickupSound)
	{
		UGameplayStatics::PlaySound2D(this, PickupSound);
	}

	if (TutorialWidgetClass)
	{
		APlayerController* PC = Cast<APlayerController>(Picker->GetController());
		TutorialWidget = PC ? CreateWidget<UUserWidget>(PC, TutorialWidgetClass) : nullptr;
		if (TutorialWidget)
		{
			TutorialWidget->AddToViewport();
			if (TutorialDisplaySeconds > 0.f)
			{
				FTimerHandle Handle;
				GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]()
				{
					if (TutorialWidget) TutorialWidget->RemoveFromParent();
					TutorialWidget = nullptr;
				}), TutorialDisplaySeconds, false);
			}
		}
	}

	SB_LOG(LogTemp, Display, TEXT("%s: 能力 %s を取得"), *GetName(), *UEnum::GetValueAsString(Ability));
	OnPickedUp(Picker);
}
