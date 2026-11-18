
#include "CursorController.h"
#include "EnhancedInputSubsystems.h"
#include "ReticleWidget.h"
#include "MyPrimaryDataAsset.h"
#include "BallLog.h"
#include "string"


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
		SB_LOG(LogTemp, Error, TEXT("UEnhancedInputLocalPlayerSubsystemの取得に失敗しました"));
		return;
	}
	
	// レティクルを取得し、画面上に配置する
	if (ReticleWidget != nullptr)
	{
		ReticleWidgetPtr = CreateWidget<UReticleWidget>(this, ReticleWidget);
		ReticleWidgetPtr->AddToViewport();
	}
	else
	{
		SB_LOG(LogTemp, Error, TEXT("ReticleWidgetの取得に失敗しました"));
		return;
	}
	
	// 鉄球の関数をデリゲートに登録する（OnPossessが先に来ていた場合はここで登録される）
	BindSteelBallDelegates();
}

void ACursorController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	// Pawnをこのクラス内にキャッシュ
	if (!((SteelBall = Cast<ASteelBallController>(InPawn))))
	{
		SB_LOG(LogTemp, Error, TEXT("SteelBallPawnの取得に失敗しました"));
		return;
	}
	
	// 鉄球の関数をデリゲートに登録する（BeginPlayが先に来ていた場合はここで登録される）
	BindSteelBallDelegates();
}

void ACursorController::OnUnPossess()
{
	// ここでは途中でreturnしない。
	UnbindSteelBallDelegates();
	SteelBall = nullptr;

	Super::OnUnPossess();
}

void ACursorController::BindSteelBallDelegates()
{
	// ウィジェットと鉄球の両方がそろうまでは何もしない。
	// BeginPlayとOnPossessのどちらが先に来ても、後から来た方で登録される。
	if (ReticleWidgetPtr == nullptr || SteelBall == nullptr) return;
	
	FOnCursorOverReticlePoint& OnPoint = ReticleWidgetPtr->OnCursorOverReticlePoint;
	
	// 未登録のものだけ登録する（二重登録を防ぐ）
	if (!SwitchDampDelegateHandle.IsValid())
	{
		SwitchDampDelegateHandle = OnPoint.AddUObject(SteelBall, &ASteelBallController::SwitchDamp);
	}
	if (!SetIsForceEnabledDelegateHandle.IsValid())
	{
		SetIsForceEnabledDelegateHandle = OnPoint.AddWeakLambda(SteelBall, [Ball = SteelBall](bool bIsMouseInPoint){Ball->SetIsForceEnabled(!bIsMouseInPoint);});
	}
	
	// デリゲートは「出入りした瞬間」にしか呼ばれないため、登録直後に今の状態を反映しておく
	const bool bIsMouseInPoint = ReticleWidgetPtr->IsMouseInPoint();
	SteelBall->SwitchDamp(bIsMouseInPoint);
	SteelBall->SetIsForceEnabled(bIsMouseInPoint);
}

void ACursorController::UnbindSteelBallDelegates()
{
	// 登録済みのものだけ解除する
	if (ReticleWidgetPtr != nullptr)
	{
		FOnCursorOverReticlePoint& OnPoint = ReticleWidgetPtr->OnCursorOverReticlePoint;
		if (SwitchDampDelegateHandle.IsValid() && !OnPoint.Remove(SwitchDampDelegateHandle))
		{
			SB_LOG(LogTemp, Warning, TEXT("SwitchDampのデリゲートからの解除に失敗しました"));
		}
		if (SetIsForceEnabledDelegateHandle.IsValid() && !OnPoint.Remove(SetIsForceEnabledDelegateHandle))
		{
			SB_LOG(LogTemp, Warning, TEXT("SetIsForceEnabledのデリゲートからの解除に失敗しました"));
		}
	}
	// 解除できたかに関わらず、古い登録を指すHandleは捨てる
	SwitchDampDelegateHandle.Reset();
	SetIsForceEnabledDelegateHandle.Reset();
	
	if (SteelBall != nullptr)
	{
		SteelBall->SwitchDamp(false);
		SteelBall->SetIsForceEnabled(false);
	}
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
	if (SteelBall == nullptr)
	{
		SB_LOG(LogTemp, Error, TEXT("SteelBallを取得できていません"));
		return;
	}
	SteelBall->Move(CursorVector);
}

void ACursorController::ApplyTuningData()
{
	if (TuningData == nullptr) return;
	MaxForce = TuningData->MaxForce;
}

// レティクルからのマウスの座標へのベクトルを返す
void ACursorController::UpdateCursorVector()
{
	// マウスのスクリーン座標を取得する
	FVector2D mousePosition;
	
	// マウスが画面内にあるかどうか
	const bool bIsMouseInScreen = GetMousePosition(mousePosition.X, mousePosition.Y);
	
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
	GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Blue, FString::Printf(TEXT("IsMouseInCircle: %d"), ReticleWidgetPtr->IsMouseInCircle()));
	GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Blue, FString::Printf(TEXT("IsMouseInPoint: %d"), ReticleWidgetPtr->IsMouseInPoint()));
	
	// マウスがサークル外にある時、ForceのベクトルのスカラーをMaxに補正する
	const TCHAR* ClampState = nullptr;
	if (!ReticleWidgetPtr->IsMouseInCircle())
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
	CursorVector = bIsMouseInScreen ? ResultVector : FVector2D(0, 0);
}

FVector2D ACursorController::ClampScalarToMax(FVector2D Vector) const
{
	// `GetSafeNormal()`でベクトルの長さを1にしてから、MaxForceをかけて長さがMaxForceのベクトルを作成する
	const FVector2D ClampedVector = Vector.GetSafeNormal() * MaxForce;
	return ClampedVector;
}
