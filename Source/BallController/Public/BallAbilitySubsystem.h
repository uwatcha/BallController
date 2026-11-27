// Author: Claude（ファイル全体を Claude が実装。README.md「実装者の記録」参照）
// 取得した能力を覚えておく World Subsystem。鉄球の Pawn はリスポーンで作り直されるので、能力の取得状態はここに置く。
// レベルを開き直すとリセットされる。

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BallAbilitySubsystem.generated.h"

/** 取得アイテムで手に入る能力。能力を増やすときはここに足す */
UENUM(BlueprintType)
enum class EBallAbility : uint8
{
	MagneticForce,
	SpeedBooster,
};

UCLASS()
class BALLCONTROLLER_API UBallAbilitySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Ability")
	void UnlockAbility(EBallAbility Ability) { Unlocked.Add(Ability); }

	UFUNCTION(BlueprintPure, Category="Ability")
	bool IsAbilityUnlocked(EBallAbility Ability) const { return Unlocked.Contains(Ability); }

private:
	UPROPERTY()
	TSet<EBallAbility> Unlocked;
};
