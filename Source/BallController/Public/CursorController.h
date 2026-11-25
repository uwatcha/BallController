// Author: User + Claude（User 主体。一部 Claude が実装。README.md「実装者の記録」参照）
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "SteelBallPawn.h"
#include "CursorController.generated.h"

class UReticleWidget;
class UMyPrimaryDataAsset;

UCLASS(PrioritizeCategories=("Parameter", "Input", "UI"))
class BALLCONTROLLER_API ACursorController : public APlayerController
{
	GENERATED_BODY()

private:
	// スクリーンの中心座標。画面の大きさの変化に対応するため、毎フレーム更新する。
	FIntPoint ScreenCenterPosition;
	UPROPERTY()
	ASteelBallPawn* SteelBall;

	// 画面中央から見たカーソルの相対ベクトル
	FVector2D CursorVector;

	// CurSorVectorを計算して更新する処理
	void UpdateCursorVector();
	
	// OnCursorOverReticlePoint デリゲートに登録する関数のHandle
	FDelegateHandle SwitchDampDelegateHandle;
	FDelegateHandle SetIsForceEnabledDelegateHandle;
	
	// デバッグ入力のバインドのHandle（二重バインド防止と解除用）
	// IA_DebugKill は鉄球ごとにバインドし直すため、Unpossess時に解除する
	uint32 DebugKillBindingHandle = 0;
	// IA_DebugRespawn はController自身の関数なので一度だけバインドする
	bool bIsDebugRespawnBound = false;
	
	// 鉄球の関数をReticleWidgetのデリゲートに登録する。BeginPlay, OnPossessから呼ぶ
	void BindSteelBallDelegates();
	// 登録した関数をデリゲートから解除し、鉄球の状態を元に戻す。OnUnPossessから呼ぶ
	void UnbindSteelBallDelegates();
	
	// 画面中央に表示されるレティクルのUI
	// BPから設定するための変数。スクリプト中では使用しない
	UPROPERTY(EditAnywhere, Category="UI")
	TSubclassOf<UReticleWidget> ReticleWidget;
	// レティクルUIをポインタとして保持したもの
	// スクリプト中でこのUIを使用するときはこちらを用いる
	UPROPERTY()
	TObjectPtr<UReticleWidget> ReticleWidgetPtr;

	// 調整値をまとめたDataAsset
	// パラメータは全てDataAssetで調整し、ApplyTuningData() で下のフィールドにコピーされる
	UPROPERTY(EditAnywhere, Category="Parameter")
	TObjectPtr<UMyPrimaryDataAsset> TuningData;

	// 以下はTuningDataから反映される値。ここでは編集せず、DataAsset側で編集する

	// カーソルがReticle外に行った時のForce
	UPROPERTY(VisibleAnywhere, Category="Parameter|FromTuningData")
	float MaxForce = 100;

	// TuningDataの値を上のフィールドにコピーする
	void ApplyTuningData();

	// デバッグ用の入力マッピング
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
	UInputMappingContext* IMC_Debug;

	// デバッグ用の即キル入力
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
	UInputAction* IA_DebugKill;

	// デバッグ用の即リスポーン入力
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
	UInputAction* IA_DebugRespawn;

	// ForceのベクトルのスカラーがMaxを超えないように補正する
	FVector2D ClampScalarToMax(FVector2D Vector) const;
	
	// [Claude] 外周円の内側で、中心点に近いほどForceの上限を小さくする (TuningData の ForceCurve を使う)
	// [Claude] OutSpeedRate には速度の上限の倍率 (SpeedCurve か ForceCurve の値) を返す
	FVector2D ScaleByForceCurve(FVector2D Vector, float& OutSpeedRate) const;
	
	// [Claude] 鉄球に渡す速度の上限の倍率。外周円の外では 1
	float SpeedRate = 1.f;
	
	// [Claude] カーソルが外周円のどこにあるか (0 = 中心, 1 = 外周円)。取得できなければ負の値
	float GetCursorDistanceRatio() const;
	
	// [Claude] WBP_Reticle の中で外周円になっているウィジェットの名前 (階層パネルの名前)
	UPROPERTY(EditAnywhere, Category="UI")
	FName ReticleCircleWidgetName = TEXT("Circle");
	
	void OnDebugRespawn();

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaTime) override;

public:
	ACursorController();
};
