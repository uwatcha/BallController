// Fill out your copyright notice in the Description page of Project Settings.

#include "SteelBallController.h"

#include "CursorController.h"
#include "EnhancedInputComponent.h" // 入力イベントと処理関数を紐づける
#include "EnhancedInputSubsystems.h" // プレイヤーごとの入力設定を管理する

#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"

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
	UpdateSphereMeshMass();
}

void ASteelBallController::BeginPlay()
{
	Super::BeginPlay();
	CursorController = Cast<ACursorController>(GetController());
	if (CursorController == nullptr) UE_LOG(LogTemp, Error, TEXT("CursorControllerの取得に失敗"));
}

void ASteelBallController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Move(CursorController->GetCursorVector());
	UpdateSphereMeshMass();
}

/**
 * SteelBallを加速度を与えることで動かす
 * @param InputVector ACursorController::GetCursorVector()で取得する、マウスによる鉄球の入力ベクトル
 */
void ASteelBallController::Move(const FVector2D& InputVector)
{
	// コントローラーの取得に失敗した時の処理
	if (Controller == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(3, 0.f, FColor::Red, TEXT("Controllerが無効です。"));
		return;
	}
	
	// Controller->Pawn->ルートのコンポーネントからSteelBallのモデルを取得。問題があれば関数を即終了
	UPrimitiveComponent* SphereMesh = Cast<UPrimitiveComponent>(Controller->GetPawn()->GetRootComponent());
	if (SphereMesh == nullptr || !SphereMesh->IsSimulatingPhysics()) return;
	
	// カメラのYaw角 (垂直軸に対する回転角度) を取得する。移動方向ベクトルを求めるときの基準に用いる
	const FRotator CameraRotation = Controller->GetControlRotation();
	const FRotator YawRotation = FRotator(0, CameraRotation.Yaw, 0);
	
	// カメラから見た、X, Y軸方向の基準となるベクトルを作成する
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	
	// ワールドでは前方がX、右方向がYなので、入力ベクトルを調整してからForceを加えるVectorを作成する
	FVector2D WorldInputVector = FVector2D(-InputVector.Y, InputVector.X);
	FVector ForceVector = (ForwardDirection * WorldInputVector.X) + (RightDirection * WorldInputVector.Y);
	
	SphereMesh->AddForce(ForceVector * ForceVectorScale, NAME_None, UseAcceleration);
	
	GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Blue, "WorldInputVector: "+WorldInputVector.ToString());
}

void ASteelBallController::UpdateSphereMeshMass()
{
	// UPrimitiveComponentの取得処理
	USceneComponent* rootComponent = this->GetRootComponent();
	if (rootComponent == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("rootComponentの取得に失敗しました。"));
		return;
	}
	UPrimitiveComponent* pc = Cast<UPrimitiveComponent>(rootComponent);
	if (pc == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("UPrimitiveComponentの取得に失敗しました。"));
		return;
	}
	
	pc->SetMassOverrideInKg(NAME_None, Mass, true);
	GEngine->AddOnScreenDebugMessage(4, 0.f, FColor::Blue, "Mass: "+FString::FromInt(pc->GetMass())+"kg");
}