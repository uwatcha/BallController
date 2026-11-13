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
		
		// Reticleを配置、マウス位置情報のフラグを取得するためにBPを取得
		UPROPERTY(EditAnywhere, Category="UI")
		TSubclassOf<UReticleWidget> ReticleWidget;
		TObjectPtr<UReticleWidget> ReticleWidgetPtr;
	
		// マウスがReticle外に行った時のForce
		UPROPERTY(EditAnywhere, Category="Parameter", meta=(UIMin=0, UIMax=3000, ClampMin=0, ClampMax=3000))
		float MaxForce = 800;
	
		// マウスがReticleの中心点にある時にブレーキのためにかける力
		UPROPERTY(EditAnywhere, Category="Parameter", meta=(UIMin=0, UIMax=2000, ClampMin=0, ClampMax=2000))
		float FrictionForce = 100;
	
		// デバッグ用の入力マッピング
		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
		UInputMappingContext* IMC_Debug;
	
		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
		UInputAction* IA_DebugKill;
	
		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input", meta=(AllowPrivateAccess="true"))
		UInputAction* IA_DebugRespawn;
	
		FVector2D ClampScalarToMax(FVector2D Vector);
		
		FVector2D CutForceAtMaxSpeed(FVector2D Vector, float MaxSpeed);


	protected:
		virtual void BeginPlay() override;
		virtual void Tick(float DeltaTime) override;
	
	public:
		ACursorController();
		FVector2D GetCursorVector() const { return CursorVector; }
};
