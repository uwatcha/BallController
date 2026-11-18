// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "SteelBallController.h"
#include "CursorController.generated.h"

class UReticleWidget;
class UMyPrimaryDataAsset;

UCLASS()
class BALLCONTROLLER_API ACursorController : public APlayerController
{
	GENERATED_BODY()

private:
	// スクリーンの中心座標。画面の大きさの変化に対応するため、毎フレーム更新する。
	FIntPoint ScreenCenterPosition;
	UPROPERTY()
	ASteelBallController* SteelBall;

	// 画面中央から見たカーソルの相対ベクトル
	FVector2D CursorVector;

	// CurSorVectorを計算して更新する処理
	void UpdateCursorVector();
	
	// OnCursorOverReticlePoint デリゲートに登録する関数のHandle
	FDelegateHandle SwitchDampDelegateHandle;
	FDelegateHandle SetIsForceEnabledDelegateHandle;
	
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

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaTime) override;

public:
	ACursorController();
};
