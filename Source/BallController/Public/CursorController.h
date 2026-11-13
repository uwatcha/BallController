// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "CursorController.generated.h"

class UReticleWidget;

/**
 * 
 */
UCLASS()
class BALLCONTROLLER_API ACursorController : public APlayerController
{
	GENERATED_BODY()

	private:
		// スクリーンの中心座標。画面の大きさの変化に対応するため、毎フレーム更新する。
		FIntPoint ScreenCenterPosition;
	
		// 画面中央のレティクルから見たカーソルの相対ベクトル
		FVector2D CursorVector;
		
		// CurSorVectorを計算して更新する処理
		void UpdateCursorVector();
		
		UPROPERTY(EditAnywhere, Category="UI")
		TSubclassOf<UReticleWidget> ReticleWidget;
	
		TObjectPtr<UReticleWidget> ReticleWidgetPtr;
	
		// デバッグ用の入力マッピング
		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
		UInputMappingContext* IMC_Debug;
	
		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
		UInputAction* IA_DebugKill;
	
		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
		UInputAction* IA_DebugRespawn;


	protected:
		virtual void BeginPlay() override;
		virtual void Tick(float DeltaTime) override;
	
	public:
		ACursorController();
		FVector2D GetCursorVector() const { return CursorVector; }
};
