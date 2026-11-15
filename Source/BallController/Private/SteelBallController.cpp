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
	
	// SphereMeshの取得
	if (!((SphereMesh = Cast<UPrimitiveComponent>(this->GetRootComponent()))))
	{
		SB_LOG(LogTemp, Error, TEXT("SphereMeshの取得に失敗"));
	}
	
	ApplyTuningData();
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
	MaxSpeed = TuningData->MaxSpeed;
	Mass = TuningData->Mass;
	ApplySphereMeshMass();
}

void ASteelBallController::ApplySphereMeshMass() const
{
	if (SphereMesh == nullptr)
	{
		SB_LOG(LogTemp, Error, TEXT("SphereMeshを取得できていません。"));
		return;
	}
	SphereMesh->SetMassOverrideInKg(NAME_None, Mass, true);
	GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Blue, "Mass: "+FString::FromInt(SphereMesh->GetMass())+"kg");
}

void ASteelBallController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
#if WITH_EDITOR
	// エディタではプレイ中にDataAssetを編集しても即反映されるよう、毎フレームコピーする
	ApplyTuningData();
	
	// 速度の画面表示
	if (SphereMesh == nullptr)
	{
		SB_LOG(LogTemp, Error, TEXT("SphereMeshを取得できていません。"));
		return;
	}
	GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Blue, FString::Printf(TEXT("Speed: %f"), GetSphereMeshSpeed()));
	GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Blue, FString::Printf(TEXT("MaxSpeed: %f"), MaxSpeed));
#endif
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
		GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Red, TEXT("Controllerが無効です。"));
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
	
	// MaxSpeedに至っている時、Forceを0にする
	
	// マウスがReticle中心点にある時、Dampingを上げる
	
	// ワールドでは前方がX、右方向がYなので、入力ベクトルを調整してからForceを加えるVectorを作成する
	const FVector2D WorldInputVector = FVector2D(-InputVector.Y, InputVector.X);
	
	const FVector2D CutVector = RemoveAccelerationAtMaxSpeed(WorldInputVector);
	
	const FVector ForceVector = (ForwardDirection * CutVector.X) + (RightDirection * CutVector.Y);
	
	SphereMesh->AddForce(ForceVector * ForceVectorScale, NAME_None, UseAcceleration);
	
	GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Blue, "WorldInputVector: "+WorldInputVector.ToString());
}

void ASteelBallController::Die()
{
	
}

FVector2D ASteelBallController::RemoveAccelerationAtMaxSpeed(FVector2D InputVector) const
{
	// SteelBallのスピードがMaxSpeedを超えていれば、(0, 0)のベクトルを返す。
	// そうでなければ、引数をそのまま返す。
	const TCHAR* CutState;
	FVector2D ResultVector;
	if (GetSphereMeshSpeed() >= MaxSpeed)
	{
		// 入力ベクトルの a 方向のベクトルを取り出す (b)
		// b と a の向きが同じなら、b を 0 にする
		
		// 現在の速度ベクトル
		const FVector CurrentVelocity = SphereMesh->GetPhysicsLinearVelocity();
		
		// InputVectorの、CurrentVelocityの成分を取り出す
		FVector InputAlongVelocityVector = FVector(InputVector, 0.f).ProjectOnTo(CurrentVelocity);
		
		// InputVectorの進行方向成分の正負を判定するため、内積を計算する
		const float InnerProduct = FVector2D::DotProduct(FVector2D(CurrentVelocity).GetSafeNormal(), FVector2D(InputAlongVelocityVector).GetSafeNormal());
		
		// InputVector に進行方向の成分が含まれていれば、それを除去したものを入力としてMoveに渡す
		// 正規化したベクトルの内積は、2ベクトル間が0度なら1に、180度なら-1になる
		if (InnerProduct == 1)
		{
			ResultVector = FVector2D(FVector::VectorPlaneProject(FVector(InputVector, 0), InputAlongVelocityVector.GetSafeNormal()));
		}
		else if (InnerProduct == -1)
		{
			ResultVector = InputVector;
		}
		else
		{
			ResultVector = InputVector;
			SB_LOG(LogTemp, Error, TEXT("計算ミスってる"));
		}
		
		CutState = TEXT("CutState: enable");
	}
	else
	{
		ResultVector = InputVector;
		CutState = TEXT("CutState: disable");
	}
	GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Blue, CutState);
	GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Blue, FString::Printf(TEXT("AccelerationRemovedVector: %s"), *ResultVector.ToString()));
	return ResultVector;
}

float ASteelBallController::GetSphereMeshSpeed() const
{
	if (SphereMesh == nullptr)
	{
		SB_LOG(LogTemp, Error, TEXT("SphereMeshが取得できていません"));
		return 0.0f;
	}
	
	return SphereMesh->GetPhysicsLinearVelocity().Size();
}