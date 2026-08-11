// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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
	// Sets default values for this pawn's properties
	ASteelBallController();

private:
	// EditAnywhere: Detailsパネルで設定できる
	// BlueprintReadOnly: BPのグラフで取得できるが、値の書き換えはできない
	// AllowPrivateAccess: C++でprivateにしていてもBPに公開できる
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* InputMappingContext;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	void Move(const FInputActionValue& Value);
	
};
