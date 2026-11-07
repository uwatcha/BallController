// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CursorController.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
// UPROPATYなどのUE専用の記述をコンパイルできるようにするためのもの
// StudyMemo: ~~.generated.hは最後にincludeする必要がある
#include "SteelBallController.generated.h"

class UInputMappingContext;
class UInputAction;

UCLASS()
class BALLCONTROLLER_API ASteelBallController : public APawn
{
	GENERATED_BODY()

public:
	ASteelBallController();

private:
	ACursorController* CursorController;
	
protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;
	
	void Move(const FVector2D& InputVector);
	
};
