#include "ABallGameMode.h"

#include "SteelBallController.h"
#include "Kismet/GameplayStatics.h"
#include "BallLog.h"

class ASteelBallController;

ABallGameMode::ABallGameMode()
{
	PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	
}

bool ABallGameMode::RespawnPlayer(AController* AController)
{
	// レベル中にSteelBallが生存していれば死ぬ処理を呼び出す
	if (APawn* Pawn = Cast<APlayerController>(AController)->GetPawn())
	{
		if (ASteelBallController* SteelBallController = Cast<ASteelBallController>(Pawn))
		{
			SteelBallController->Die();
		}
		else
		{
			SB_LOG(LogTemp, Error, TEXT("SteelBallControllerの取得に失敗しました。"));
			return false;
		}	
	}
	else
	{
		SB_LOG(LogTemp, Error, TEXT("Pawnの取得に失敗しました。"));
		return false;
	}
	
	// GameModeが再スタートさせる
	this->RestartPlayer(PlayerController);
	return true;
}
