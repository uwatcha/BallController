// Author: Claude（Claude が実装。README.md「実装者の記録」参照）
// DebugDrawSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Debug/DebugDrawService.h"
#include "DebugDrawSubsystem.generated.h"

class UCanvas;
class APlayerController;

UCLASS()
class UDebugDrawSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	// 画面（Canvas）に 2D で描くためのコールバック。毎フレーム呼ばれる
	void DrawAxisGizmo(UCanvas* Canvas, APlayerController* PC);

	FDelegateHandle DebugDrawHandle;
};
