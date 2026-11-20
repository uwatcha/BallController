#include "ABallGameMode.h"

#include "SteelBallPawn.h"
#include "Kismet/GameplayStatics.h"
#include "BallLog.h"

class ASteelBallPawn;

ABallGameMode::ABallGameMode()
{
}

bool ABallGameMode::RespawnPlayer(AController* AController)
{
	if (AController == nullptr)
	{
		SB_LOG(LogTemp, Error, TEXT("RespawnPlayer: Controllerがnullです。"));
		return false;
	}
	
	// レベル中にSteelBallが生存していれば、先に死ぬ処理を呼び出す。
	// Kill済み（Pawnがいない）の場合はそのまま復活させる。
	if (ASteelBallPawn* SteelBallPawn = Cast<ASteelBallPawn>(AController->GetPawn()))
	{
		SteelBallPawn->Die();
	}
	
	// GameModeが再スタートさせる
	this->RestartPlayer(AController);
	return true;
}
