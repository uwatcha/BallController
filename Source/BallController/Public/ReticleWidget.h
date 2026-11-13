// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReticleWidget.generated.h"

/**
 * 
 */
UCLASS()
class BALLCONTROLLER_API UReticleWidget : public UUserWidget
{
	GENERATED_BODY()
	
	public:
		/// マウスが画面上のレティクルの外周円の中に入っているかどうか
		/// 超えていれば、CursorControllerでForceが上限より強くならないようにする
		UPROPERTY(BlueprintReadWrite)
		bool IsMouseInCircle;
	
		/// マウス画面上のレティクルの中心点に重なっているかどうか
		/// 重なっていれば、摩擦力を発生させ徐々に減速させる
		UPROPERTY(BlueprintReadWrite)	
		bool IsMouseInPoint;
};
