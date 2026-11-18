#include "ReticleWidget.h"

void UReticleWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	if (bWasMouseInPoint != bIsMouseInPoint)
	{
		OnCursorOverReticlePoint.Broadcast(bIsMouseInPoint);
		bWasMouseInPoint = bIsMouseInPoint;
	}
}
