// Author: User + Claude（User 主体。一部 Claude が実装。README.md「実装者の記録」参照）

#include "CursorController.h"

#include "ABallGameMode.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "ReticleWidget.h"
#include "MyPrimaryDataAsset.h"
#include "BallLog.h"
#include "Blueprint/WidgetLayoutLibrary.h" // [Claude] GetMousePositionOnPlatform
#include "string"


class ABallGameMode;
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
	if (!((SteelBall = Cast<ASteelBallPawn>(InPawn))))
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
		SwitchDampDelegateHandle = OnPoint.AddUObject(SteelBall, &ASteelBallPawn::SwitchDamp);
	}
	if (!SetIsForceEnabledDelegateHandle.IsValid())
	{
		SetIsForceEnabledDelegateHandle = OnPoint.AddWeakLambda(SteelBall, [Ball = SteelBall](bool bIsCursorInPoint){Ball->SetIsForceEnabled(!bIsCursorInPoint);});
	}
	
	// デリゲートは「出入りした瞬間」にしか呼ばれないため、登録直後に今の状態を反映しておく
	const bool bIsCursorInPoint = ReticleWidgetPtr->IsCursorInPoint();
	SteelBall->SwitchDamp(bIsCursorInPoint);
	SteelBall->SetIsForceEnabled(!bIsCursorInPoint);
	
	// InputのBind
	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(this->InputComponent);
	if (EIC == nullptr)
	{
		SB_LOG(LogTemp, Error, TEXT("EnhancedInputComponentを取得できませんでした。"));
		return;
	}
	// 鉄球が変わるたびにバインドし直す（古い鉄球のバインドはUnbindSteelBallDelegatesで解除済み）
	if (DebugKillBindingHandle == 0)
	{
		DebugKillBindingHandle = EIC->BindAction(
			IA_DebugKill,
			ETriggerEvent::Started,
			SteelBall,
			&ASteelBallPawn::Die
		).GetHandle();
	}
	// Controller自身の関数なので、リスポーンしても一度だけ
	if (!bIsDebugRespawnBound)
	{
		EIC->BindAction(
			IA_DebugRespawn,
			ETriggerEvent::Started,
			this,
			&ACursorController::OnDebugRespawn
			);
		bIsDebugRespawnBound = true;
	}
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
	
	// 古い鉄球に向いた IA_DebugKill のバインドを解除する
	if (DebugKillBindingHandle != 0)
	{
		if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
		{
			EIC->RemoveBindingByHandle(DebugKillBindingHandle);
		}
		DebugKillBindingHandle = 0;
	}
	
	if (SteelBall != nullptr)
	{
		SteelBall->SwitchDamp(false);
		SteelBall->SetIsForceEnabled(true);
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
	
	// 毎フレーム、GetCursorVectorでカーソルの座標を取得し、CursorVectorに入れる
	UpdateCursorVector();
	GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Blue, "CursorVector: "+CursorVector.ToString());
	
	// SteelBallを動かす
	if (SteelBall == nullptr)
	{
		SB_LOG(LogTemp, Error, TEXT("SteelBallを取得できていません"));
		return;
	}
	SteelBall->SetMaxSpeedRate(SpeedRate); // [Claude] カーソルの距離で速度の上限を変える
	SteelBall->Move(CursorVector);
}

void ACursorController::ApplyTuningData()
{
	if (TuningData == nullptr) return;
	MaxForce = TuningData->MaxForce;
}

// レティクルからのカーソルの座標へのベクトルを返す
void ACursorController::UpdateCursorVector()
{
	// カーソルのスクリーン座標を取得する
	FVector2D cursorPosition;
	
	// カーソルが画面内にあるかどうか
	const bool bIsCursorInScreen = GetMousePosition(cursorPosition.X, cursorPosition.Y);
	
	// レティクルからのカーソルの相対ベクトルを計算する
	FVector2D reticlePosition = ScreenCenterPosition;
	
	// カーソル座標の、レティクルからの相対ベクトルを計算する
	FVector2D relativeVector = cursorPosition - reticlePosition;
	
	FVector2D ResultVector = relativeVector;
	
	// ====== カーソルがReticle外にある時、スカラーをMaxForceにする ======
	if (ReticleWidgetPtr == nullptr)
	{
		SB_LOG(LogTemp, Error, TEXT("ReticleWidgetが取得できていない"));
		return;
	}
	
	// IsCursorInCircle, IsCursorInPointの表示
	GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Blue, FString::Printf(TEXT("IsCursorInCircle: %d"), ReticleWidgetPtr->IsCursorInCircle()));
	GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Blue, FString::Printf(TEXT("IsCursorInPoint: %d"), ReticleWidgetPtr->IsCursorInPoint()));
	
	// カーソルがサークル外にある時、ForceのベクトルのスカラーをMaxに補正する
	const TCHAR* ClampState = nullptr;
	SpeedRate = 1.f; // [Claude] 外周円の外では MaxSpeed のまま
	if (!ReticleWidgetPtr->IsCursorInCircle())
	{
		ResultVector = ClampScalarToMax(relativeVector);
		ClampState = TEXT("enable");
	}
	else
	{
		// [Claude] サークル内では、中心点に近いほどForceの上限を小さくする
		ResultVector = ScaleByForceCurve(relativeVector, SpeedRate);
		ClampState = TEXT("disable");
	}
	GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Blue, FString::Printf(TEXT("ClampScalarToMax: %s"), ClampState));
	// ======                                             ======
	
	// CursorVectorを更新する。カーソルがスクリーン外にあれば、ベクトルは(0, 0)。
	CursorVector = bIsCursorInScreen ? ResultVector : FVector2D(0, 0);
}

FVector2D ACursorController::ClampScalarToMax(FVector2D Vector) const
{
	// `GetSafeNormal()`でベクトルの長さを1にしてから、MaxForceをかけて長さがMaxForceのベクトルを作成する
	const FVector2D ClampedVector = Vector.GetSafeNormal() * MaxForce;
	return ClampedVector;
}

// [Claude] ここから GetCursorDistanceRatio() まで Claude が実装
FVector2D ACursorController::ScaleByForceCurve(FVector2D Vector, float& OutSpeedRate) const
{
	OutSpeedRate = 1.f;
	// 外周円が見つからない時や、カーブが空の時は、今までどおりのベクトルを返す (理由を画面に出す)
	if (TuningData == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Red, TEXT("ForceCurve未使用: BP_CursorController の TuningData が未設定"));
		return Vector;
	}
	const FRichCurve* Curve = TuningData->ForceCurve.GetRichCurveConst();
	if (Curve == nullptr || Curve->GetNumKeys() == 0)
	{
		GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Red, TEXT("ForceCurve未使用: DataAsset の Force Curve にキーがない"));
		return Vector;
	}
	const float Ratio = GetCursorDistanceRatio();
	if (Ratio < 0.f)
	{
		GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Red, FString::Printf(TEXT("ForceCurve未使用: WBP_Reticle に「%s」という名前のウィジェットがない、または大きさが0"), *ReticleCircleWidgetName.ToString()));
		return Vector;
	}
	
	// カーブから MaxForce にかける倍率を読む (横軸 0〜1 の外に出ないようにする)
	const float ClampedRatio = FMath::Clamp(Ratio, 0.f, 1.f);
	const float Rate = FMath::Max(0.f, Curve->Eval(ClampedRatio));
	
	// 速度の上限の倍率。チェックボックスが ON なら Force と同じカーブを使う
	const FRichCurve* SpeedCurve = TuningData->bUseForceCurveForSpeed ? Curve : TuningData->SpeedCurve.GetRichCurveConst();
	if (SpeedCurve != nullptr && SpeedCurve->GetNumKeys() > 0)
	{
		OutSpeedRate = FMath::Max(0.f, SpeedCurve->Eval(ClampedRatio));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Red, TEXT("SpeedCurve未使用: DataAsset の Speed Curve にキーがない"));
	}
	GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Blue, FString::Printf(TEXT("CursorDistanceRatio: %.2f, ForceRate: %.2f, SpeedRate: %.2f"), Ratio, Rate, OutSpeedRate));
	
	// 向きはカーソルの方向のまま、長さを MaxForce * 倍率 にする
	return Vector.GetSafeNormal() * MaxForce * Rate;
}

float ACursorController::GetCursorDistanceRatio() const
{
	if (ReticleWidgetPtr == nullptr) return -1.f;
	const UWidget* Circle = ReticleWidgetPtr->GetWidgetFromName(ReticleCircleWidgetName);
	if (Circle == nullptr)
	{
		return -1.f;
	}
	// WBP_Reticle の IsMouseIn と同じ計算: 外周円のローカル座標でのカーソルと中心の距離 / 半径
	const FGeometry& Geometry = Circle->GetCachedGeometry();
	const FVector2D LocalSize = Geometry.GetLocalSize();
	const float Radius = LocalSize.X * 0.5f;
	if (Radius <= UE_KINDA_SMALL_NUMBER) return -1.f;
	const FVector2D LocalCursor = Geometry.AbsoluteToLocal(UWidgetLayoutLibrary::GetMousePositionOnPlatform());
	return FVector2D::Distance(LocalCursor, LocalSize * 0.5f) / Radius;
}
// [Claude] ここまで

void ACursorController::OnDebugRespawn()
{
	// 呼ばれたことを画面に3秒だけ表示する
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("IA_DebugRespawn: OnDebugRespawn"));
	ABallGameMode* BallGameMode = GetWorld()->GetAuthGameMode<ABallGameMode>();
	if (BallGameMode == nullptr)
	{
		SB_LOG(LogTemp, Error, TEXT("BallGameModeの取得に失敗しました"));
		return;
	}
	if (!BallGameMode->RespawnPlayer(this))
	{
		SB_LOG(LogTemp, Error, TEXT("Respawnに失敗しました"));
	}
}
