// Author: User（User が実装）
#include "ReticleWidget.h"

void UReticleWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	if (bWasCursorInPoint != bIsCursorInPoint)
	{
		OnCursorOverReticlePoint.Broadcast(bIsCursorInPoint);
		bWasCursorInPoint = bIsCursorInPoint;
	}
}
