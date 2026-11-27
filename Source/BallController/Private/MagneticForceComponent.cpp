// Author: Claude（ファイル全体を Claude が実装。README.md「実装者の記録」参照）
// マグネティックフォース（MF）。鉄球の BP に付けるコンポーネント。

#include "MagneticForceComponent.h"

#include "BallLog.h"
#include "BallIronBox.h"
#include "BallAbilitySubsystem.h"
#include "MyPrimaryDataAsset.h"
#include "DrawDebugHelpers.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

UMagneticForceComponent::UMagneticForceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// 物理の計算の前に力をかける
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		FieldMesh = SphereMesh.Object;
	}
}

void UMagneticForceComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Cast<APawn>(GetOwner()) == nullptr)
	{
		SB_LOG(LogTemp, Error, TEXT("MagneticForceComponent: 鉄球の Pawn に付けてください"));
	}
	if (TuningData == nullptr)
	{
		SB_LOG(LogTemp, Warning, TEXT("MagneticForceComponent: TuningData が空なので MF は効きません"));
	}

	// 半透明の領域。鉄球は転がって回転するので、アタッチせずに毎フレーム位置だけ合わせる
	FieldComponent = NewObject<UStaticMeshComponent>(GetOwner(), TEXT("MagneticField"));
	FieldComponent->SetStaticMesh(FieldMesh);
	FieldComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FieldComponent->SetGenerateOverlapEvents(false);
	FieldComponent->SetCastShadow(false);
	FieldComponent->SetHiddenInGame(true);
	FieldComponent->RegisterComponent();
	if (FieldMaterial)
	{
		FieldMID = UMaterialInstanceDynamic::Create(FieldMaterial, this);
		FieldComponent->SetMaterial(0, FieldMID);
	}
	else
	{
		SB_LOG(LogTemp, Warning, TEXT("MagneticForceComponent: FieldMaterial が空です（領域が不透明の白い球になります）"));
	}

	TryBindInput();
}

void UMagneticForceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindInput();
	Held.Reset();
	if (FieldComponent)
	{
		FieldComponent->DestroyComponent();
		FieldComponent = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void UMagneticForceComponent::TryBindInput()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (BoundController.Get() == PC) return; // バインド済み（または両方 null）
	UnbindInput();
	if (PC == nullptr) return;

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PC->InputComponent);
	if (EIC == nullptr) return; // まだ作られていない。次の Tick で再挑戦する

	if (IMC_Ability)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(IMC_Ability, IMCPriority);
		}
	}
	else
	{
		SB_LOG(LogTemp, Warning, TEXT("MagneticForceComponent: IMC_Ability が未設定です"));
	}

	if (IA_MagneticForce)
	{
		BindingHandles.Add(EIC->BindAction(IA_MagneticForce, ETriggerEvent::Started, this, &UMagneticForceComponent::OnMagnetStarted).GetHandle());
		BindingHandles.Add(EIC->BindAction(IA_MagneticForce, ETriggerEvent::Completed, this, &UMagneticForceComponent::OnMagnetCompleted).GetHandle());
	}
	else
	{
		SB_LOG(LogTemp, Warning, TEXT("MagneticForceComponent: IA_MagneticForce が未設定です"));
	}
	BoundController = PC;
}

void UMagneticForceComponent::UnbindInput()
{
	if (APlayerController* PC = BoundController.Get())
	{
		if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PC->InputComponent))
		{
			for (const uint32 Handle : BindingHandles) EIC->RemoveBindingByHandle(Handle);
		}
		if (IMC_Ability)
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
			{
				Subsystem->RemoveMappingContext(IMC_Ability);
			}
		}
	}
	BindingHandles.Reset();
	BoundController.Reset();
	bIsActive = false;
}

bool UMagneticForceComponent::IsAbilityUsable() const
{
	if (bUsableWithoutPickup) return true;
	const UBallAbilitySubsystem* Abilities = GetWorld() ? GetWorld()->GetSubsystem<UBallAbilitySubsystem>() : nullptr;
	return Abilities && Abilities->IsAbilityUnlocked(EBallAbility::MagneticForce);
}

void UMagneticForceComponent::OnMagnetStarted()
{
	// まだ取得していなければ発動しない
	bIsActive = IsAbilityUsable();
}

void UMagneticForceComponent::OnMagnetCompleted()
{
	bIsActive = false;
	// 離したら全て手放す。対象はそのまま物理で転がって止まる
	Held.Reset();
}

UPrimitiveComponent* UMagneticForceComponent::GetBallBody() const
{
	return GetOwner() ? Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent()) : nullptr;
}

void UMagneticForceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TryBindInput();

	UPrimitiveComponent* Ball = GetBallBody();
	if (Ball == nullptr) return;
	const FVector BallLocation = Ball->GetComponentLocation();

	if (bIsActive && TuningData)
	{
		GrabTargetsInRange(BallLocation);
		PullTargets(Ball);
	}
	UpdateField(BallLocation);

#if WITH_EDITOR
	if (bShowDebug && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Cyan,
			FString::Printf(TEXT("MF: %s  Held: %d"), !IsAbilityUsable() ? TEXT("LOCKED") : (bIsActive ? TEXT("ON") : TEXT("OFF")), Held.Num()));
	}
#endif
}

void UMagneticForceComponent::GrabTargetsInRange(const FVector& BallLocation)
{
	const float Range = FMath::Max(0.f, TuningData->MFRange);
	if (Range <= 0.f) return;

	// 物理で動く物（PhysicsBody など動的な物）だけを、球の範囲で探す。壁越しでもつかむ
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MagneticForce), false, GetOwner());
	GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		BallLocation,
		FQuat::Identity,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllDynamicObjects),
		FCollisionShape::MakeSphere(Range),
		Params);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		UPrimitiveComponent* Comp = Overlap.GetComponent();
		const AActor* Actor = Overlap.GetActor();
		if (Comp == nullptr || Actor == nullptr || Actor == GetOwner()) continue;
		if (!Actor->ActorHasTag(TuningData->MFTargetTag)) continue;
		// 射程は「中心どうしの距離」で判定する（重なりは箱の端が入っただけでも当たるため）
		if (FVector::Dist(Comp->GetComponentLocation(), BallLocation) > Range) continue;
		// 固定中の鉄箱は、ここで初めて物理で動くようにする
		if (ABallIronBox* Box = Cast<ABallIronBox>(const_cast<AActor*>(Actor)))
		{
			Box->UnlockByMagnet();
		}
		if (!Comp->IsSimulatingPhysics()) continue;
		Held.AddUnique(Comp);
	}
}

float UMagneticForceComponent::EvalPullRate(float Distance, float Hold, float Range, float Release) const
{
	if (Distance <= Hold) return 0.f;

	const FRuntimeFloatCurve& Curve = (Distance <= Range) ? TuningData->MFInRangeCurve : TuningData->MFOutOfRangeCurve;
	const float X = (Distance <= Range)
		? (Distance - Hold) / FMath::Max(Range - Hold, 1.f)
		: (Distance - Range) / FMath::Max(Release - Range, 1.f);
	const FRichCurve* Rich = Curve.GetRichCurveConst();
	const float Y = Curve.ExternalCurve ? Curve.ExternalCurve->GetFloatValue(X) : (Rich ? Rich->Eval(X) : 1.f);
	return FMath::Max(0.f, Y);
}

void UMagneticForceComponent::PullTargets(UPrimitiveComponent* Ball)
{
	const float Hold = FMath::Max(0.f, TuningData->MFHoldDistance);
	const float Range = FMath::Max(Hold, TuningData->MFRange);
	const float Release = FMath::Max(Range, TuningData->MFReleaseDistance);
	const bool bHorizontal = TuningData->bMFHorizontalOnly;

	const FVector BallLocation = Ball->GetComponentLocation();
	const FVector BallVelocity = Ball->GetPhysicsLinearVelocity();

	for (int32 i = Held.Num() - 1; i >= 0; --i)
	{
		UPrimitiveComponent* Target = Held[i].Get();
		if (Target == nullptr || !Target->IsSimulatingPhysics())
		{
			Held.RemoveAt(i);
			continue;
		}

		FVector ToBall = BallLocation - Target->GetComponentLocation();
		FVector RelVelocity = Target->GetPhysicsLinearVelocity() - BallVelocity;
		if (bHorizontal)
		{
			ToBall.Z = 0.f;
			RelVelocity.Z = 0.f;
		}
		const float Distance = ToBall.Size();
		if (Distance > Release)
		{
			Held.RemoveAt(i);
			continue;
		}

		const float Rate = EvalPullRate(Distance, Hold, Range, Release);
		const FVector Dir = Distance > KINDA_SMALL_NUMBER ? ToBall / Distance : FVector::ZeroVector;
		// 引っ張る加速度 + 球との速さの差を打ち消すブレーキ
		const FVector Accel = Dir * TuningData->MFMaxPullAcceleration * Rate - RelVelocity * TuningData->MFPullDamping;
		// bAccelChange = true: 加速度として扱う（対象の重さに関係なく同じ動きになる）
		Target->AddForce(Accel, NAME_None, true);

#if WITH_EDITOR
		if (bShowDebug)
		{
			const FColor LineColor = Distance <= Range ? FColor::Cyan : FColor::Orange;
			DrawDebugLine(GetWorld(), BallLocation, Target->GetComponentLocation(), LineColor, false, 0.f, 0, 3.f);
			DrawDebugString(GetWorld(), Target->GetComponentLocation() + FVector(0.f, 0.f, 80.f),
				FString::Printf(TEXT("%.0fcm x%.2f"), Distance, Rate), nullptr, LineColor, 0.f);
		}
#endif
	}
}

void UMagneticForceComponent::UpdateField(const FVector& BallLocation)
{
	if (FieldComponent == nullptr) return;

	const bool bShow = bIsActive && TuningData && TuningData->MFRange > 0.f;
	FieldComponent->SetHiddenInGame(!bShow);
	if (!bShow) return;

	// エンジン標準の Sphere は直径 100cm
	FieldComponent->SetWorldLocationAndRotation(BallLocation, FQuat::Identity);
	FieldComponent->SetWorldScale3D(FVector(TuningData->MFRange * 2.f / 100.f));
	if (FieldMID)
	{
		FieldMID->SetVectorParameterValue(TEXT("Color"), TuningData->MFFieldColor);
		FieldMID->SetScalarParameterValue(TEXT("Opacity"), TuningData->MFFieldOpacity);
	}
}
