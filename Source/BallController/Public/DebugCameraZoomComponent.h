// Author: Claude（ファイル全体を Claude が実装。README.md「実装者の記録」参照）
// デバッグ用のカメラズーム。PlayerController（BP_CursorController）に付けて使う。
// キー長押しで SpringArm の TargetArmLength を縮める/伸ばし、キー単押しで元の長さに戻す。

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DebugCameraZoomComponent.generated.h"

class UInputAction;
class USpringArmComponent;
struct FInputActionValue;

UCLASS(ClassGroup=(Debug), meta=(BlueprintSpawnableComponent), PrioritizeCategories=("DebugZoom"))
class BALLCONTROLLER_API UDebugCameraZoomComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDebugCameraZoomComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// ====== 入力（IMC_Debug にキーを割り当てる） ======

	// 長押しでズームイン（カメラが近づく）、単押しでリセット
	UPROPERTY(EditAnywhere, Category="DebugZoom|Input")
	TObjectPtr<UInputAction> IA_DebugZoomIn;

	// 長押しでズームアウト（カメラが離れる）、単押しでリセット
	UPROPERTY(EditAnywhere, Category="DebugZoom|Input")
	TObjectPtr<UInputAction> IA_DebugZoomOut;

	// ====== 調整値 ======

	// 1秒あたりに変わるアームの長さ
	UPROPERTY(EditAnywhere, Category="DebugZoom", meta=(ClampMin="0"))
	float ZoomSpeed = 1500.f;

	// アームの最短の長さ（これより近づかない）
	UPROPERTY(EditAnywhere, Category="DebugZoom", meta=(ClampMin="0"))
	float MinArmLength = 200.f;

	// アームの最長の長さ（これより離れない）
	UPROPERTY(EditAnywhere, Category="DebugZoom", meta=(ClampMin="0"))
	float MaxArmLength = 5000.f;

	// この秒数より長く押したら「長押し」、短ければ「単押し」（リセット）
	UPROPERTY(EditAnywhere, Category="DebugZoom", meta=(ClampMin="0"))
	float HoldThreshold = 0.2f;

	// 今の長さを画面に表示するか
	UPROPERTY(EditAnywhere, Category="DebugZoom")
	bool bShowArmLength = true;

	// ====== 内部状態 ======

	// 操作中の SpringArm（Pawn が変わったら取り直す）
	TWeakObjectPtr<USpringArmComponent> SpringArm;
	// SpringArm を見つけたときの長さ。リセットでこの値に戻す
	float DefaultArmLength = 0.f;

	// 押している方向（+1: ズームアウト、-1: ズームイン、0: 押していない）
	float ZoomDirection = 0.f;
	// 押し始めてからの秒数
	float HeldTime = 0.f;

	// 入力をバインドしたか。InputComponent ができるまで Tick で試し続ける
	bool bIsInputBound = false;
	TArray<uint32> BindingHandles;

	void TryBindInput();
	// 今の Pawn の SpringArm を返す。Pawn が変わっていたら取り直し、初期の長さを覚える
	USpringArmComponent* FindSpringArm();

	void OnZoomInStarted() { StartPress(-1.f); }
	void OnZoomOutStarted() { StartPress(+1.f); }
	void OnZoomInCompleted() { EndPress(-1.f); }
	void OnZoomOutCompleted() { EndPress(+1.f); }

	void StartPress(float Direction);
	void EndPress(float Direction);
	void ResetZoom();
};
