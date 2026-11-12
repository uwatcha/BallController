DECLARE_MULTICAST_DELEGATE_OneParam(FOnPawnDied, AController*);

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ABallGameMode.generated.h"

UCLASS()
class BALLCONTROLLER_API ABallGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
	public:
		ABallGameMode();
		bool RespawnPlayer(AController*);
		FOnPawnDied OnPawnDied;
	
	private:
		APlayerController* PlayerController;
};
