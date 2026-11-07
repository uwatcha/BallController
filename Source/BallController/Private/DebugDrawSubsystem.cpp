// DebugDrawSubsystem.cpp
#include "DebugDrawSubsystem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "CanvasItem.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

void UDebugDrawSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
#if !UE_BUILD_SHIPPING
	// "Game" ShowFlag が有効なビューポートの描画時に、Canvas 付きで呼んでもらう
	DebugDrawHandle = UDebugDrawService::Register(
		TEXT("Game"), FDebugDrawDelegate::CreateUObject(this, &UDebugDrawSubsystem::DrawAxisGizmo));
#endif
}

void UDebugDrawSubsystem::Deinitialize()
{
#if !UE_BUILD_SHIPPING
	UDebugDrawService::Unregister(DebugDrawHandle);
#endif
	Super::Deinitialize();
}

void UDebugDrawSubsystem::DrawAxisGizmo(UCanvas* Canvas, APlayerController* PC)
{
#if !UE_BUILD_SHIPPING
	// 自分のワールドのプレイヤーの描画だけ扱う（エディタビューポートなどは PC が null）
	if (!Canvas || !PC || PC->GetWorld() != GetWorld() || !PC->PlayerCameraManager)
	{
		return;
	}

	// カメラの向きから、画面の右方向・上方向をワールド空間で求める
	const FRotator CamRot = PC->PlayerCameraManager->GetCameraRotation();
	const FRotationMatrix CamMatrix(CamRot);
	const FVector CamRight = CamMatrix.GetUnitAxis(EAxis::Y);
	const FVector CamUp    = CamMatrix.GetUnitAxis(EAxis::Z);

	// ギズモの基点：画面左下から少し内側
	const float Margin = 40.f;
	const float Length = 30.f;
	const FVector2D Origin(Margin, Canvas->ClipY - Margin);

	auto DrawAxis = [&](const FVector& WorldDir, const FLinearColor& Color, const TCHAR* Label)
	{
		// ワールド方向を画面平面に射影（画面の Y は下向きが正なので Up は符号反転）
		const FVector2D ScreenDir(FVector::DotProduct(WorldDir, CamRight),
		                          -FVector::DotProduct(WorldDir, CamUp));
		const FVector2D End = Origin + ScreenDir * Length;

		FCanvasLineItem Line(Origin, End);
		Line.SetColor(Color);
		Line.LineThickness = 1.5f;
		Canvas->DrawItem(Line);

		FCanvasTextItem Text(End + ScreenDir.GetSafeNormal() * 5.f - FVector2D(4.f, 8.f),
		                     FText::FromString(Label), GEngine->GetSmallFont(), Color);
		Canvas->DrawItem(Text);
	};

	// カメラ固定・垂直軸なしなので X / Y のみ
	DrawAxis(FVector::ForwardVector, FLinearColor::Red,   TEXT("X"));
	DrawAxis(FVector::RightVector,   FLinearColor::Green, TEXT("Y"));
#endif
}
