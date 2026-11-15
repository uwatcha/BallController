// Fill out your copyright notice in the Description page of Project Settings.


#include "CursorController.h"
#include "EnhancedInputSubsystems.h"
#include "ReticleWidget.h"
#include "MyPrimaryDataAsset.h"
#include "BallLog.h"


class InputAction;
class InputMappingContext;

ACursorController::ACursorController()
{
}

void ACursorController::BeginPlay()
{
	Super::BeginPlay();
	ApplyTuningData();
	if (TuningData == nullptr) SB_LOG(LogTemp, Warning, TEXT("CursorController: TuningDataが未設定です。既定値を使います。"));
	
	// IMCをPlayerControllerに登録する処理
	if (UEnhancedInputLocalPlayerSubsystem* LocalPlayer = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(this->GetLocalPlayer()))
	{
		LocalPlayer->AddMappingContext(IMC_Debug, 0);
	} 
	else
	{
		SB_LOG(LogTemp, Error, TEXT("UEnhancedInputLocalPlayerSubsystemの取得に失敗しました。"));
		return;
	}
	
	// レティクルを取得し、画面上に配置する
	if (ReticleWidget != nullptr)
	{
		ReticleWidgetPtr = CreateWidget<UReticleWidget>(this, ReticleWidget);
		ReticleWidgetPtr->AddToViewport();
	}
	
	// Pawnをこのクラス内にキャッシュする
	SteelBall = Cast<ASteelBallController>(this->GetPawn());
}

void ACursorController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
#if WITH_EDITOR
	// エディタではプレイ中にDataAssetを編集しても即反映されるよう、毎フレームコピーする
	ApplyTuningData();
#endif
	
	// スクリーンの中心座標を取得、更新
	FIntPoint viewportSize;
	GetViewportSize(viewportSize.X, viewportSize.Y);
	ScreenCenterPosition = viewportSize / 2;
	
	// 毎フレーム、GetCursorVectorでマウスの座標を取得し、CursorVectorに入れる
	UpdateCursorVector();
	GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Blue, "CursorVector: "+CursorVector.ToString());
	
	// SteelBallを動かす
	SteelBall->Move(CursorVector);
}

void ACursorController::ApplyTuningData()
{
	if (TuningData == nullptr) return;
	MaxForce = TuningData->MaxForce;
	FrictionForce = TuningData->FrictionForce;
}

// レティクルからのマウスの座標へのベクトルを返す
void ACursorController::UpdateCursorVector()
{
	// マウスのスクリーン座標を取得する
	FVector2D mousePosition;
	
	// マウスが画面内にあるかどうか
	const bool IsMouseInScreen = GetMousePosition(mousePosition.X, mousePosition.Y);
	
	// レティクルからのマウスの相対ベクトルを計算する
	FVector2D reticlePosition = ScreenCenterPosition;
	
	// マウス座標の、レティクルからの相対ベクトルを計算する
	FVector2D relativeVector = mousePosition - reticlePosition;
	
	FVector2D ResultVector = relativeVector;
	
	// ====== マウスがReticle外にある時、スカラーをMaxForceにする ======
	if (ReticleWidgetPtr == nullptr)
	{
		SB_LOG(LogTemp, Error, TEXT("ReticleWidgetが取得できていない"));
		return;
	}
	
	// IsMouseInCircle, IsMouseInPointの表示
	GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Blue, FString::Printf(TEXT("IsMouseInCircle: %d"), ReticleWidgetPtr->IsMouseInCircle));
	GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Blue, FString::Printf(TEXT("IsMouseInPoint: %d"), ReticleWidgetPtr->IsMouseInPoint));
	
	// マウスがサークル外にある時、ForceのベクトルのスカラーをMaxに補正する
	const TCHAR* ClampState = nullptr;
	if (!ReticleWidgetPtr->IsMouseInCircle)
	{
		ResultVector = ClampScalarToMax(relativeVector);
		ClampState = TEXT("enable");
	}
	else
	{
		ClampState = TEXT("disable");
	}
	GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Blue, FString::Printf(TEXT("ClampScalarToMax: %s"), ClampState));
	// ======                                             ======
	
	// CursorVectorを更新する。マウスがスクリーン外にあれば、ベクトルは(0, 0)。
	CursorVector = IsMouseInScreen ? ResultVector : FVector2D(0, 0);
}

FVector2D ACursorController::ClampScalarToMax(FVector2D Vector) const
{
	// `GetSafeNormal()`でベクトルの長さを1にしてから、MaxForceをかけて長さがMaxForceのベクトルを作成する
	const FVector2D ClampedVector = Vector.GetSafeNormal() * MaxForce;
	return ClampedVector;
}
