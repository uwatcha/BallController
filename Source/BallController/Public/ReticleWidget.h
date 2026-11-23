// Author: User（User が実装）
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReticleWidget.generated.h"

/// カーソルがPointを出た or 入った時に発火するデリゲート。
/// @param bIsCursorInPoint true: 入った時、false: 出た時
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCursorOverReticlePoint, bool bIsCursorInPoint);

UCLASS()
class BALLCONTROLLER_API UReticleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/// カーソルがPointを出た or 入った時に発火するデリゲート。
	FOnCursorOverReticlePoint OnCursorOverReticlePoint;
	
	// フラグのゲッター
	bool IsCursorInCircle() const { return bIsCursorInCircle; }
	bool IsCursorInPoint() const { return bIsCursorInPoint; }
	
protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/// カーソルが画面上のレティクルの外周円の中に入っているかどうか
	/// 超えていれば、CursorControllerでForceが上限より強くならないようにする
	UPROPERTY(BlueprintReadWrite, meta=(AllowPrivateAccess = "true"))
	bool bIsCursorInCircle;

	/// カーソルが画面上のレティクルの中心点に重なっているかどうか
	/// 重なっていれば、摩擦力を発生させ徐々に減速させる
	UPROPERTY(BlueprintReadWrite, meta=(AllowPrivateAccess = "true"))
	bool bIsCursorInPoint;
	
	// 前回のループで、CursorがPointの中にあったかどうか
	bool bWasCursorInPoint = false;
};
