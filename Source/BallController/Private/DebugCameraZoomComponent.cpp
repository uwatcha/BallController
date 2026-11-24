// Author: Claude（ファイル全体を Claude が実装。README.md「実装者の記録」参照）

#include "DebugCameraZoomComponent.h"

#include "EnhancedInputComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "BallLog.h"

UDebugCameraZoomComponent::UDebugCameraZoomComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UDebugCameraZoomComponent::BeginPlay()
{
	Super::BeginPlay();
	if (Cast<APlayerController>(GetOwner()) == nullptr)
	{
		SB_LOG(LogTemp, Error, TEXT("DebugCameraZoomComponent: PlayerController に付けてください"));
	}
	TryBindInput();
}

void UDebugCameraZoomComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
	{
		if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PC->InputComponent))
		{
			for (const uint32 Handle : BindingHandles) EIC->RemoveBindingByHandle(Handle);
		}
	}
	BindingHandles.Reset();
	bIsInputBound = false;
	Super::EndPlay(EndPlayReason);
}

void UDebugCameraZoomComponent::TryBindInput()
{
	if (bIsInputBound) return;
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (PC == nullptr) return;
	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PC->InputComponent);
	if (EIC == nullptr) return; // まだ作られていない。次の Tick で再挑戦する

	if (IA_DebugZoomIn)
	{
		BindingHandles.Add(EIC->BindAction(IA_DebugZoomIn, ETriggerEvent::Started, this, &UDebugCameraZoomComponent::OnZoomInStarted).GetHandle());
		BindingHandles.Add(EIC->BindAction(IA_DebugZoomIn, ETriggerEvent::Completed, this, &UDebugCameraZoomComponent::OnZoomInCompleted).GetHandle());
	}
	else
	{
		SB_LOG(LogTemp, Warning, TEXT("DebugCameraZoomComponent: IA_DebugZoomIn が未設定です"));
	}
	if (IA_DebugZoomOut)
	{
		BindingHandles.Add(EIC->BindAction(IA_DebugZoomOut, ETriggerEvent::Started, this, &UDebugCameraZoomComponent::OnZoomOutStarted).GetHandle());
		BindingHandles.Add(EIC->BindAction(IA_DebugZoomOut, ETriggerEvent::Completed, this, &UDebugCameraZoomComponent::OnZoomOutCompleted).GetHandle());
	}
	else
	{
		SB_LOG(LogTemp, Warning, TEXT("DebugCameraZoomComponent: IA_DebugZoomOut が未設定です"));
	}
	bIsInputBound = true;
}

USpringArmComponent* UDebugCameraZoomComponent::FindSpringArm()
{
	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (Pawn == nullptr)
	{
		SpringArm.Reset();
		return nullptr;
	}
	// Pawn が変わった（リスポーンした）ら取り直す
	if (!SpringArm.IsValid() || SpringArm->GetOwner() != Pawn)
	{
		SpringArm = Pawn->FindComponentByClass<USpringArmComponent>();
		if (SpringArm.IsValid())
		{
			DefaultArmLength = SpringArm->TargetArmLength;
		}
	}
	return SpringArm.Get();
}

void UDebugCameraZoomComponent::StartPress(float Direction)
{
	ZoomDirection = Direction;
	HeldTime = 0.f;
}

void UDebugCameraZoomComponent::EndPress(float Direction)
{
	// 別のキーに押し替えたあとで前のキーを離した場合は無視する
	if (ZoomDirection != Direction) return;
	if (HeldTime < HoldThreshold)
	{
		ResetZoom();
	}
	ZoomDirection = 0.f;
	HeldTime = 0.f;
}

void UDebugCameraZoomComponent::ResetZoom()
{
	if (USpringArmComponent* Arm = FindSpringArm())
	{
		Arm->TargetArmLength = DefaultArmLength;
	}
}

void UDebugCameraZoomComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TryBindInput();
	USpringArmComponent* Arm = FindSpringArm();
	if (Arm == nullptr) return;

	if (ZoomDirection != 0.f)
	{
		HeldTime += DeltaTime;
		// 長押しと判定されてから動かす（単押しでカメラが少し動いてしまわないように）
		if (HeldTime >= HoldThreshold)
		{
			const float MinLen = FMath::Min(MinArmLength, MaxArmLength);
			const float MaxLen = FMath::Max(MinArmLength, MaxArmLength);
			Arm->TargetArmLength = FMath::Clamp(Arm->TargetArmLength + ZoomDirection * ZoomSpeed * DeltaTime, MinLen, MaxLen);
		}
	}

	if (bShowArmLength && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Cyan,
			FString::Printf(TEXT("DebugZoom ArmLength: %.0f (Default %.0f)"), Arm->TargetArmLength, DefaultArmLength));
	}
}
