// Author: Claude（ファイル全体を Claude が実装。README.md「実装者の記録」参照）
// マグネティックフォース（MF）。鉄球の BP（BP_SteelBall）に付けるコンポーネント。
// キーを押している間だけ有効。射程内の「引き寄せ対象のタグが付いた、物理で動く物」を全てつかみ、球の方へ引っ張る。
// 力は対象にだけかけるので、球の動きは対象の重さの影響を受けない。
// 発動中は射程の大きさの半透明の球を表示する。調整値は全て DataAsset（Ability|MagneticForce）で設定する。
// Pawn（SteelBallPawn）のコードは変更していない。

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MagneticForceComponent.generated.h"

class UInputAction;
class UInputMappingContext;
class UMyPrimaryDataAsset;
class UPrimitiveComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class APlayerController;

UCLASS(ClassGroup=(Ability), meta=(BlueprintSpawnableComponent), PrioritizeCategories=("MagneticForce"))
class BALLCONTROLLER_API UMagneticForceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMagneticForceComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 発動中か（キーを押している間 true）
	UFUNCTION(BlueprintPure, Category="MagneticForce")
	bool IsMagnetActive() const { return bIsActive; }

	// 今 MF を使えるか（取得アイテムで手に入れたか、bUsableWithoutPickup がオン）
	UFUNCTION(BlueprintPure, Category="MagneticForce")
	bool IsAbilityUsable() const;

	// 今つかんでいる対象の数
	UFUNCTION(BlueprintPure, Category="MagneticForce")
	int32 GetHeldCount() const { return Held.Num(); }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 調整値を読むデータアセット（鉄球と同じ MyPrimaryDataAsset を入れる）
	UPROPERTY(EditAnywhere, Category="MagneticForce")
	TObjectPtr<UMyPrimaryDataAsset> TuningData;

	// オン: 取得アイテムがなくても最初から MF を使える（テスト用）。オフ: 「か」の取得アイテムに触れるまで使えない
	UPROPERTY(EditAnywhere, Category="MagneticForce")
	bool bUsableWithoutPickup = false;

	// MF を発動する Input Action（値の型: Digital (bool)）。押している間だけ有効
	UPROPERTY(EditAnywhere, Category="MagneticForce|Input")
	TObjectPtr<UInputAction> IA_MagneticForce;

	// IA_MagneticForce にキーを割り当てた Input Mapping Context。このコンポーネントが登録する
	UPROPERTY(EditAnywhere, Category="MagneticForce|Input")
	TObjectPtr<UInputMappingContext> IMC_Ability;

	// IMC_Ability の優先度（IMC_Debug は 0）
	UPROPERTY(EditAnywhere, Category="MagneticForce|Input")
	int32 IMCPriority = 1;

	// 半透明の領域のマテリアル（"Color" ベクトルと "Opacity" スカラーのパラメータを持つ、半透明のもの）
	UPROPERTY(EditAnywhere, Category="MagneticForce|Field")
	TObjectPtr<UMaterialInterface> FieldMaterial;

	// 領域に使うメッシュ（直径 100cm の球。エンジン標準の Sphere）
	UPROPERTY(EditAnywhere, Category="MagneticForce|Field")
	TObjectPtr<UStaticMesh> FieldMesh;

	// エディタで遊んでいる間、状態を画面に出し、球から対象へ線を描く
	UPROPERTY(EditAnywhere, Category="MagneticForce|Debug")
	bool bShowDebug = true;

private:
	// キーを押しているか
	bool bIsActive = false;

	// つかんでいる対象
	TArray<TWeakObjectPtr<UPrimitiveComponent>> Held;

	// 半透明の領域（BeginPlay で作る）
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> FieldComponent;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FieldMID;

	// 入力のバインド（Pawn が PlayerController に操作されるまで Tick で試し続ける）
	TWeakObjectPtr<APlayerController> BoundController;
	TArray<uint32> BindingHandles;
	void TryBindInput();
	void UnbindInput();

	void OnMagnetStarted();
	void OnMagnetCompleted();

	// 鉄球の物理の本体（Pawn のルート）
	UPrimitiveComponent* GetBallBody() const;

	// 射程内の対象を探して Held に足す
	void GrabTargetsInRange(const FVector& BallLocation);
	// Held の対象を引っ張る。解除距離より遠い対象は離す
	void PullTargets(UPrimitiveComponent* Ball);
	// 距離から、引っ張る強さの倍率を求める（射程内 / 射程外のカーブ）
	float EvalPullRate(float Distance, float Hold, float Range, float Release) const;

	void UpdateField(const FVector& BallLocation);
};
