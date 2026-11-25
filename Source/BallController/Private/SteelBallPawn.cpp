// Author: User + Claude（User 主体。一部 Claude が実装。README.md「実装者の記録」参照）
// Fill out your copyright notice in the Description page of Project Settings.

#include "SteelBallPawn.h"

#include "CursorController.h"
#include "MyPrimaryDataAsset.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h" // [Claude] ApplyBallSize
#include "GameFramework/SpringArmComponent.h" // [Claude] ApplyCameraArmLength

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
ASteelBallPawn::ASteelBallPawn()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ASteelBallPawn::PossessedBy(AController* Controller)
{
	Super::PossessedBy(Controller);
	
	// SphereMeshの取得
	if (!((SphereMesh = Cast<UPrimitiveComponent>(this->GetRootComponent()))))
	{
		SB_LOG(LogTemp, Error, TEXT("SphereMeshの取得に失敗"));
	}
	
	bIsForceEnabled = true;
	
	ApplyTuningData();
}

void ASteelBallPawn::BeginPlay()
{
	Super::BeginPlay();
	ApplyTuningData();
	if (TuningData == nullptr) SB_LOG(LogTemp, Warning, TEXT("SteelBallPawn: TuningDataが未設定です。既定値を使います。"));
}

void ASteelBallPawn::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyTuningData();
}

void ASteelBallPawn::ApplyTuningData()
{
	// 未設定なら既定値のまま (警告はBeginPlayで1回だけ出す)
	if (TuningData == nullptr) return;
	ForceVectorScale = TuningData->ForceVectorScale;
	UseAcceleration = TuningData->UseAcceleration;
	MaxSpeed = TuningData->MaxSpeed;
	Mass = TuningData->Mass;
	LinearDamping = TuningData->LinearDamping;
	AngularDamping = TuningData->AngularDamping;
	DampType = TuningData->DampType;
	GroundCheckMargin = TuningData->GroundCheckMargin; // [Claude]
	GravityZ = TuningData->GravityZ; // [Claude]
	ApplySphereMeshMass();
	ApplyBallSize();        // [Claude]
	ApplyCameraArmLength(); // [Claude]
}

// [Claude] ここから ApplyCameraArmLength() まで Claude が実装
void ASteelBallPawn::ApplyBallSize()
{
	// ルートの StaticMesh を拡大縮小すると、Mesh の当たり判定 (Simple Collision) も一緒に拡大縮小される
	const UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(GetRootComponent());
	if (Mesh == nullptr || Mesh->GetStaticMesh() == nullptr) return;
	
	// 拡大率 1 の時の直径。/Engine/BasicShapes/Sphere なら 100cm
	const float BaseDiameter = Mesh->GetStaticMesh()->GetBounds().BoxExtent.Z * 2.f;
	if (BaseDiameter <= UE_KINDA_SMALL_NUMBER) return;
	
	const float TargetScale = TuningData->BallDiameter / BaseDiameter;
	const float CurrentScale = GetActorScale3D().Z;
	// 毎フレーム呼ばれるので、大きさが変わった時だけ処理する
	if (FMath::IsNearlyEqual(TargetScale, CurrentScale, 1.e-4f)) return;
	
	// 中心を基準に拡大すると底が床にめり込むので、半径が増えた分だけ上に (減った分だけ下に) ずらして底の高さを保つ
	const float DeltaRadius = (TargetScale - CurrentScale) * BaseDiameter * 0.5f;
	AddActorWorldOffset(FVector(0.f, 0.f, DeltaRadius), false, nullptr, ETeleportType::TeleportPhysics);
	SetActorScale3D(FVector(TargetScale));
	// 質量は SetMassOverrideInKg で固定しているので、大きさを変えても Mass のまま
}

void ASteelBallPawn::ApplyCameraArmLength()
{
	// 0 の時は BP の SpringArm の値をそのまま使う
	if (TuningData->CameraArmLength <= 0.f) return;
	// 値が変わった時だけ反映する (デバッグズーム中の長さを毎フレーム戻さないため)
	if (FMath::IsNearlyEqual(LastAppliedCameraArmLength, TuningData->CameraArmLength)) return;
	
	// SpringArm は BP_SteelBallPawn にだけあるので、型で探す
	USpringArmComponent* SpringArm = FindComponentByClass<USpringArmComponent>();
	if (SpringArm == nullptr) return;
	SpringArm->TargetArmLength = TuningData->CameraArmLength;
	LastAppliedCameraArmLength = TuningData->CameraArmLength;
}
// [Claude] ここまで

void ASteelBallPawn::ApplySphereMeshMass() const
{
	if (SphereMesh == nullptr)
	{
		SB_LOG(LogTemp, Error, TEXT("SphereMeshを取得できていません。"));
		return;
	}
	SphereMesh->SetMassOverrideInKg(NAME_None, Mass, true);
	GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Blue, "Mass: "+FString::FromInt(SphereMesh->GetMass())+"kg");
}

void ASteelBallPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	ApplyGravity(); // [Claude]
	
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
	// [Claude] 速度のベクトルと大きさ (cm/s)
	const FVector Velocity = SphereMesh->GetPhysicsLinearVelocity();
	GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Yellow, FString::Printf(TEXT("Velocity: %s  Size: %.1f"),
		*Velocity.ToCompactString(), Velocity.Size()));
	GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Blue, FString::Printf(TEXT("MaxSpeed: %f (Current %f)"), MaxSpeed, GetCurrentMaxSpeed())); // [Claude] Current を追加
	GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Blue, FString::Printf(TEXT("AngularDamping: %f"), AngularDamping));
	GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Blue, FString::Printf(TEXT("IsForceEnable: %s"), bIsForceEnabled ? TEXT("True") : TEXT("False")));
	// GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Blue, FString::Printf(TEXT("AngularDamping: %f"), AngularDamping));
#endif
}

/**
 * SteelBallを加速度を与えることで動かす
 * @param InputVector ACursorController::GetCursorVector()で取得する、カーソルによる鉄球の入力ベクトル
 */
void ASteelBallPawn::Move(const FVector2D& InputVector) const
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
	
	FVector ForceVector;
	// [Claude] 空中 (IsGrounded() が false) の時は Force を渡さない
	const bool bIsGrounded = IsGrounded();
	GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Blue, FString::Printf(TEXT("IsGrounded: %s"), bIsGrounded ? TEXT("True") : TEXT("False")));
	if (bIsForceEnabled && bIsGrounded)
	{
		// ワールドでは前方がX、右方向がYなので、入力ベクトルを調整してからForceを加えるVectorを作成する
		const FVector2D WorldInputVector = FVector2D(-InputVector.Y, InputVector.X);
	
		const FVector2D CutVector = RemoveAccelerationAtMaxSpeed(WorldInputVector);
	
		ForceVector = (ForwardDirection * CutVector.X) + (RightDirection * CutVector.Y);
	}
	else
	{
		ForceVector = FVector(0,0,0);
	}
	
	SphereMesh->AddForce(ForceVector * ForceVectorScale, NAME_None, UseAcceleration);
	
	GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Blue, "ForceVector: "+ForceVector.ToString());
	// [Claude] 実際に AddForce に渡した値 (ForceVector × ForceVectorScale) のベクトルと大きさ
	const FVector AppliedForce = ForceVector * ForceVectorScale;
	GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Yellow, FString::Printf(TEXT("Force: %s  Size: %.1f%s"),
		*AppliedForce.ToCompactString(), AppliedForce.Size(), UseAcceleration ? TEXT(" (Accel)") : TEXT("")));
}

// [Claude] IsGrounded() は Claude が実装
bool ASteelBallPawn::IsGrounded() const
{
	if (SphereMesh == nullptr) return false;
	
	// Bounds は拡大縮小後の大きさなので、DataAsset で直径を変えても半径が追従する
	const FVector Start = SphereMesh->Bounds.Origin;
	const float Radius = SphereMesh->Bounds.SphereRadius;
	const FVector End = Start - FVector(0.f, 0.f, Radius + GroundCheckMargin);
	
	// 自分自身の当たり判定には当たらないようにする
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SteelBallGroundCheck), false, this);
	FHitResult Hit;
	return GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
}

// [Claude] ApplyGravity() は Claude が実装
void ASteelBallPawn::ApplyGravity() const
{
	if (SphereMesh == nullptr || !SphereMesh->IsSimulatingPhysics()) return;
	
	// ワールドの重力は使わず、DataAsset の重力だけをかける
	if (SphereMesh->IsGravityEnabled())
	{
		SphereMesh->SetEnableGravity(false);
	}
	// 加速度として加えるので、重さに関係なく GravityZ で落ちる (普通の重力と同じ)
	SphereMesh->AddForce(FVector(0.f, 0.f, GravityZ), NAME_None, /*bAccelChange=*/true);
	GEngine->AddOnScreenDebugMessage(__LINE__, 0.f, FColor::Blue, FString::Printf(TEXT("GravityZ: %f"), GravityZ));
}

void ASteelBallPawn::Die()
{
	// 呼ばれたことを画面に3秒だけ表示する
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("IA_DebugKill: Die"));
	APlayerController* PC = Cast<APlayerController>(Controller);
	this->Destroy();
	SB_LOG(LogTemp, Display, TEXT("DIE!!"));
}

FVector2D ASteelBallPawn::RemoveAccelerationAtMaxSpeed(FVector2D InputVector) const
{
	const TCHAR* CutState;
	FVector2D ResultVector;
	if (GetSphereMeshSpeed() >= GetCurrentMaxSpeed()) // [Claude] MaxSpeed → カーソルの距離で変わる上限
	{
		// 現在の速度ベクトル
		const FVector CurrentVelocity = SphereMesh->GetPhysicsLinearVelocity();
		
		// InputVectorの、CurrentVelocityの成分を取り出す
		const FVector InputAlongVelocityVector = FVector(InputVector, 0.f).ProjectOnTo(CurrentVelocity);
		
		// InputVectorの進行方向成分の正負を判定するため、内積を計算する
		const float InnerProduct = FVector2D::DotProduct(FVector2D(CurrentVelocity).GetSafeNormal(), FVector2D(InputAlongVelocityVector).GetSafeNormal());
		
		// InputVector に進行方向の成分が含まれていれば、それを除去したものを入力としてMoveに渡す
		// 正規化したベクトルの内積は、2ベクトル間が0度なら1に、180度なら-1になる
		if (FMath::IsNearlyEqual(InnerProduct, 1.f, UE_KINDA_SMALL_NUMBER))
		{
			ResultVector = FVector2D(FVector::VectorPlaneProject(FVector(InputVector, 0), InputAlongVelocityVector.GetSafeNormal()));
		}
		else if (FMath::IsNearlyEqual(InnerProduct, -1.f, UE_KINDA_SMALL_NUMBER) || FMath::IsNearlyZero(InnerProduct, UE_KINDA_SMALL_NUMBER))
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

float ASteelBallPawn::GetSphereMeshSpeed() const
{
	if (SphereMesh == nullptr)
	{
		SB_LOG(LogTemp, Error, TEXT("SphereMeshが取得できていません"));
		return 0.0f;
	}
	
	return SphereMesh->GetPhysicsLinearVelocity().Size();
}

void ASteelBallPawn::SwitchDamp(bool bIsActive)
{
	if (SphereMesh == nullptr)
	{
		SB_LOG(LogTemp, Error, TEXT("SphereMeshが取得できていません"));
		return;
	}
	// DampTypeで選ばれた方だけを強め、もう片方は常に初期値に戻す
	// (プレイ中にDampTypeを切り替えても、前の種類の減衰が残らないようにするため)
	const bool bUseLinear = (DampType == ESteelBallDampType::Linear);
	SphereMesh->SetLinearDamping((bIsActive && bUseLinear) ? LinearDamping : INITIAL_LINEAR_DAMPING);
	SphereMesh->SetAngularDamping((bIsActive && !bUseLinear) ? AngularDamping : INITIAL_ANGULAR_DAMPING);
}