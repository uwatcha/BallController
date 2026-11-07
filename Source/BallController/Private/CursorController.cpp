// Fill out your copyright notice in the Description page of Project Settings.


#include "CursorController.h"
#include "BallController/Public/CursorController.h"
#include "Kismet/GameplayStatics.h"

ACursorController::ACursorController()
{
}

void ACursorController::BeginPlay()
{
	Super::BeginPlay();
	//TODO: PlayerControllerからマウスの情報を取得し、
	
	
	PlayerController = this;
	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	
	//画面の縦横の長さを取得
	FSceneViewProjectionData ProjectionData;
	LocalPlayer->GetProjectionData(LocalPlayer->ViewportClient->Viewport, ProjectionData);
	UE_LOG(LogTemp, Warning, TEXT("Viewport Width: %d"), ProjectionData
		.GetConstrainedViewRect().Width());
	UE_LOG(LogTemp, Warning, TEXT("Viewport Height: %d"), ProjectionData
		.GetConstrainedViewRect().Height());
	
	
}

void ACursorController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// スクリーンの中心座標を取得、更新
	FIntPoint viewportSize;
	GetViewportSize(viewportSize.X, viewportSize.Y);
	ScreenCenterPosition = viewportSize / 2;
	
	// 毎フレーム、GetCursorVectorでマウスの座標を取得し、CursorVectorに入れる
	UpdateCursorVector();
	GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Blue, "CursorVector: "+CursorVector.ToString());
	
	//TODO: BallControllerに、CursorVectorを受け取り、AddForce()に入れる処理を書く。
}

// レティクルからのマウスの座標へのベクトルを返す
void ACursorController::UpdateCursorVector()
{
	// マウスのスクリーン座標を取得する
	FVector2D mousePosition;
	GetMousePosition(mousePosition.X, mousePosition.Y);
	
	// レティクルからのマウスの相対ベクトルを計算する
	FVector2D reticlePosition = ScreenCenterPosition;
	
	// マウス座標の、レティクルからの相対ベクトルを計算し、CursorPositionに反映する
	FVector2D relativeVector = mousePosition - reticlePosition;
	CursorVector = relativeVector;
}
