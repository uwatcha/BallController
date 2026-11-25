// Author: Claude（Claude が実装。README.md「実装者の記録」参照）
// 鉄球や鉄箱（物理シミュレーション中のアクター）が乗ると押されるスイッチ。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BallSwitch.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

// 押し方の種類
UENUM(BlueprintType)
enum class EBallSwitchMode : uint8
{
	Once UMETA(DisplayName="Once（一度押せばON のまま）"),
	Keep UMETA(DisplayName="Keep（押している間だけON）"),
};

class ABallSwitch;

// スイッチの効果（扉を開けるかどうか）が切り替わったときの通知。
// グループに入っているスイッチは、グループ全体の状態（全部押されたか）を通知する。
// 扉はどれか1つのスイッチにバインドすればよい。
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBallSwitchActiveChanged, ABallSwitch*, Switch, bool, bActive);

UCLASS(PrioritizeCategories=("Switch"))
class BALLCONTROLLER_API ABallSwitch : public AActor
{
	GENERATED_BODY()

public:
	ABallSwitch();

	virtual void Tick(float DeltaSeconds) override;

	// 効果が出ているか（単体なら押されているか、グループなら全員が押されているか）
	UFUNCTION(BlueprintPure, Category="Switch")
	bool IsActive() const { return bActive; }

	// このスイッチ自体が「押された扱い」か（Once は一度押したら true のまま）
	UFUNCTION(BlueprintPure, Category="Switch")
	bool IsPressed() const { return bPressed; }

	// 効果が切り替わったときに呼ばれる（扉側でバインドする）
	UPROPERTY(BlueprintAssignable, Category="Switch")
	FOnBallSwitchActiveChanged OnActiveChanged;

protected:
	virtual void BeginPlay() override;

	// ===== エディタで設定する値 =====

	// Once: 一度押せば ON のまま / Keep: 押している間だけ ON
	UPROPERTY(EditAnywhere, Category="Switch")
	EBallSwitchMode Mode = EBallSwitchMode::Once;

	// 同じ名前を入れたスイッチ同士が1つのグループになり、全部押されると効果が出る。
	// None（空）なら単体のスイッチ。
	UPROPERTY(EditAnywhere, Category="Switch")
	FName GroupName = NAME_None;

	// この質量(kg)以上の物理アクターが乗ったときだけ押される。0 なら何でも押せる
	UPROPERTY(EditAnywhere, Category="Switch")
	float MinMassKg = 0.f;

	// 押されていない
	UPROPERTY(EditAnywhere, Category="Switch|Color")
	FLinearColor ColorReleased = FLinearColor::Red;

	// 押されているが、グループにまだ押されていないスイッチがある
	UPROPERTY(EditAnywhere, Category="Switch|Color")
	FLinearColor ColorWaitingGroup = FLinearColor::Yellow;

	// 効果が出ている（単体で押されている / グループ全員が押されている）
	UPROPERTY(EditAnywhere, Category="Switch|Color")
	FLinearColor ColorActive = FLinearColor::Green;

	// ===== コンポーネント =====

	UPROPERTY(VisibleAnywhere, Category="Switch")
	TObjectPtr<USceneComponent> Root;

	// 見た目の板（当たり判定なし。色が変わる）
	UPROPERTY(VisibleAnywhere, Category="Switch")
	TObjectPtr<UStaticMeshComponent> PlateMesh;

	// この箱の中に物理アクターが入ると押される（大きさはエディタで変えてよい）
	UPROPERTY(VisibleAnywhere, Category="Switch")
	TObjectPtr<UBoxComponent> PressArea;

private:
	// 箱の中に、押せる物理アクターがいるか調べる
	bool CheckPressedNow() const;

	// 自分の押下状態が変わったとき、グループ全員の状態を計算し直す
	void RefreshGroup();

	// グループ全体の結果を受け取って、色と通知を更新する
	void ApplyGroupState(bool bGroupAllPressed);

	void UpdateColor();

	bool bPressed = false;
	bool bActive = false;

	// 自分を含むグループのメンバー（単体なら自分だけ）
	TArray<TWeakObjectPtr<ABallSwitch>> GroupMembers;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> PlateMaterial;
};
