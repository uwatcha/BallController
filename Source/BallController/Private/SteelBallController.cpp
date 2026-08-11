// Fill out your copyright notice in the Description page of Project Settings.

#include "SteelBallController.h"
#include "EnhancedInputComponent.h" // 入力イベントと処理関数を紐づける
#include "EnhancedInputSubsystems.h" // プレイヤーごとの入力設定を管理する

#include "Engine/LocalPlayer.h"

/* StudyMemo ===============
キー入力
↓
PlayerController
↓
InputComponent
↓
登録された関数を呼ぶ
=============================*/

// Sets default values
ASteelBallController::ASteelBallController()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	UE_LOG(LogTemp, Warning, TEXT("ASteelBallController::Constructor"));
	PrimaryActorTick.bCanEverTick = true;
}

void ASteelBallController::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("ASteelBallController::BeginPlay()"));

	// InputMappingContext(キー入力と操作の割り当て)をプレイヤーに登録する処理===============================================================
	// 広い意味のControllerから、プレイヤー入力を扱うAPlayerControllerにCast
	// Castしたコントローラーを取得し、成功したら続きの処理を続行
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		// StudyMemo: -> : ポインタが指しているオブジェクトのメンバーにアクセスする
		// StudyMemo: UEが用意しているクラスは、UEが生成、破棄を管理するため、基本的にポインタ型として扱う。(Unityと同じ）
		// StudyMemo: 頭文字A: Actor系。ワールド上に存在できるもの。
		// StudyMemo: 頭文字U: UObject系。Unityでいうコンポーネントや入力管理など
		// コントローラーを取得できたら、そこから辿ってプレイヤー情報を取得
		if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			// プレイヤー情報からSubsystemを取得	
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<
				UEnhancedInputLocalPlayerSubsystem>())
			{
				Subsystem->AddMappingContext(InputMappingContext, 0);
				UE_LOG(LogTemp, Warning, TEXT("InputMappingContextを追加"));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Subsystemの取得に失敗"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("LocalPlayerの取得に失敗"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerControllerの取得に失敗"));
	}
	// ==============================================================================================================================
}

// Called every frame
void ASteelBallController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
// Pawnに入力を紐づけるための関数
void ASteelBallController::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	UE_LOG(LogTemp, Warning, TEXT("ASteelBallController::SetupPlayerInputComponent()"));

	//PlayerInputComponentをEnhancedInputComponentとして取得
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		//入力(例：前方向入力)と、それを受け付けた時に呼び出す関数を登録する
		EnhancedInputComponent->BindAction(
			// .hで宣言したInputAction
			MoveAction,
			// 入力条件を満たしている間(長押し中はtrue)
			//StudyMemo: ETriggerEventには他に、Start, Canceledなどがある
			ETriggerEvent::Triggered,
			// 呼び出したい関数を持っているオブジェクトを登録
			this,
			// 呼び出したい関数のポインタ
			&ASteelBallController::Move
		);
		UE_LOG(LogTemp, Warning, TEXT("MoveAction bound"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerInputComponentのbindに失敗"));
	}
}

void ASteelBallController::Move(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Log, TEXT("==============="));
	FVector2D Vector = Value.Get<FVector2D>();
	UE_LOG(LogTemp, Log, TEXT("Vector: %s"), *Vector.ToString());
	if (Controller == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Controllerが無効です。"))
		return;
	}
	APawn* SteelBallPawn = Controller->GetPawn();
	UE_LOG(LogTemp, Log, TEXT("SteelBallPawn: %s"), *SteelBallPawn->GetName());
	UPrimitiveComponent* SphereMesh = Cast<UPrimitiveComponent>(SteelBallPawn->GetRootComponent());
	if (SphereMesh == nullptr || !SphereMesh->IsSimulatingPhysics()) return;
	
	const FRotator Rotation = Controller->GetControlRotation();
	const FRotator YawRotation = FRotator(0, Rotation.Yaw, 0);
	UE_LOG(LogTemp, Log, TEXT("YawRotation: %s"), *YawRotation.ToString());

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	UE_LOG(LogTemp, Log, TEXT("ForwardDirection: %s"), *ForwardDirection.ToString());

	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	UE_LOG(LogTemp, Log, TEXT("RightDirection: %s"), *RightDirection.ToString());

	FVector MovementDirection = (ForwardDirection * Vector.X) + (RightDirection * Vector.Y);
	
	SphereMesh->AddForce(MovementDirection * 500000.0f);
	
	// 	const FVector Right = GetActorRightVector();
	// 	AddMovementInput(Right, Vector.X*100);
	// 	const FVector Forward = GetActorForwardVector();
	// 	AddMovementInput(Forward, Vector.Y*100);
}
