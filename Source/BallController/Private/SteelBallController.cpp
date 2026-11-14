// Fill out your copyright notice in the Description page of Project Settings.

#include "SteelBallController.h"

#include "CursorController.h"
#include "MyPrimaryDataAsset.h"
#include "Components/SphereComponent.h"

#include "Engine/LocalPlayer.h"
#include "BallLog.h"

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
	PrimaryActorTick.bCanEverTick = true;
}

void ASteelBallController::PossessedBy(AController* Controller)
{
	Super::PossessedBy(Controller);
	
	if (!((CursorController = Cast<ACursorController>(Controller))))
	{
		SB_LOG(LogTemp, Error, TEXT("CursorControllerの取得に失敗"));
		return;
	}
	
	if (!((SphereMesh = Cast<UPrimitiveComponent>(this->GetRootComponent()))))
	{
		SB_LOG(LogTemp, Error, TEXT("SphereMeshの取得に失敗"));
	}
}

void ASteelBallController::BeginPlay()
{
	Super::BeginPlay();
	ApplyTuningData();
	if (TuningData == nullptr) SB_LOG(LogTemp, Warning, TEXT("SteelBallController: TuningDataが未設定です。既定値を使います。"));
}

void ASteelBallController::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyTuningData();
}

void ASteelBallController::ApplyTuningData()
{
	// 未設定なら既定値のまま (警告はBeginPlayで1回だけ出す)
	if (TuningData == nullptr) return;
	ForceVectorScale = TuningData->ForceVectorScale;
	UseAcceleration = TuningData->UseAcceleration;
	Mass = TuningData->Mass;
}

void ASteelBallController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
#if WITH_EDITOR
	// エディタではプレイ中にDataAssetを編集しても即反映されるよう、毎フレームコピーする
	ApplyTuningData();
	
	if (SphereMesh == nullptr)
	{
		SB_LOG(LogTemp, Error, TEXT("SphereMeshを取得できていません。"));
		return;
	}
	float speed = SphereMesh->GetPhysicsLinearVelocity().Size();
	GEngine->AddOnScreenDebugMessage(6, 0.f, FColor::Blue, TEXT("Speed(cmps): "), speed);
#endif
	if (CursorController != nullptr) Move(CursorController->GetCursorVector());
	ApplySphereMeshMass();
}

/**
 * SteelBallを加速度を与えることで動かす
 * @param InputVector ACursorController::GetCursorVector()で取得する、マウスによる鉄球の入力ベクトル
 */
void ASteelBallController::Move(const FVector2D& InputVector) const
{
	// コントローラーの取得に失敗した時の処理
	if (Controller == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(3, 0.f, FColor::Red, TEXT("Controllerが無効です。"));
		return;
	}
	
	// Controller->Pawn->ルートのコンポーネントからSteelBallのモデルを取得。問題があれば関数を即終了
	// UPrimitiveComponent* SphereMesh = Cast<UPrimitiveComponent>(Controller->GetPawn()->GetRootComponent());
	if (SphereMesh == nullptr || !SphereMesh->IsSimulatingPhysics()) return;
	
	// カメラのYaw角 (垂直軸に対する回転角度) を取得する。移動方向ベクトルを求めるときの基準に用いる
	const APlayerController* PC = Cast<APlayerController>(Controller);
	if (PC == nullptr)
	{
		SB_LOG(LogTemp, Error, TEXT("PlayerControllerの取得に失敗"));
		return;
	}
	const APlayerCameraManager* PCM = PC->PlayerCameraManager;
	if (PCM == nullptr)
	{
		SB_LOG(LogTemp, Error, TEXT("PlayerCameraManagerの取得に失敗"));
		return;
	}	
	const FRotator CameraRotation = PCM->GetCameraRotation();
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

void ASteelBallController::ApplySphereMeshMass() const
{
	if (SphereMesh == nullptr)
	{
		SB_LOG(LogTemp, Error, TEXT("SphereMeshを取得できていません。"));
		return;
	}
	SphereMesh->SetMassOverrideInKg(NAME_None, Mass, true);
	GEngine->AddOnScreenDebugMessage(4, 0.f, FColor::Blue, "Mass: "+FString::FromInt(SphereMesh->GetMass())+"kg");
}

void ASteelBallController::Die()
{
	
}
