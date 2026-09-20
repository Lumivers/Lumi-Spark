#include "Lumi_SparkGameMode.h"
#include "LSPlayerController.h"
#include "LSGameState.h"
#include "Character/LSCharacterBase.h"
#include "Character/LSTeamSwitchComponent.h"
#include "Core/LSEventBus.h"
#include "Kismet/GameplayStatics.h"
#include "Weapon/LSGrenadeBase.h"

ALumi_SparkGameMode::ALumi_SparkGameMode()
{
	//指定默认玩家控制器
	PlayerControllerClass = ALSPlayerController::StaticClass();
	GameStateClass = ALSGrenadeBase::StaticClass();
	DefaultPawnClass = nullptr;
}

void ALumi_SparkGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
	// 服务端更新在线玩家数并重算动态缩放
	if (ALSGameState* GS = GetGameState<ALSGameState>())
	{
		GS->SetConnectedPlayerCount(GetNumPlayers());
	}
}

void ALumi_SparkGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	if (ALSGameState* GS = GetGameState<ALSGameState>())
	{
		//减去正在退出的玩家
		GS->SetConnectedPlayerCount(FMath::Max(1, GetNumPlayers() - 1));
	}
	
	//玩家离场后重新检测是否触发剩余全员团灭
	CheckRaidWipeCondition();
}

void ALumi_SparkGameMode::CheckRaidWipeCondition()
{
	if (!HasAuthority()) return;
	
	ALSGameState* GS = GetGameState<ALSGameState>();
	if (!GS || GS->IsRaidWiped()) return;
	
	int32 TotalActivePlayers = 0;
	int32 IncapacitatedPlayers = 0;
	
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ALSPlayerController* PC = Cast<ALSPlayerController>(It->Get());
		if (!PC) continue;
		
		TotalActivePlayers++;
		
		ALSCharacterBase* ActiveChar = nullptr;
		if (ULSTeamSwitchComponent* TeamComp = PC->GetTeamSwitchComponent())
		{
			ActiveChar = TeamComp->GetActiveCharacter();
		}
		else
		{
			ActiveChar = PC->GetPawn<ALSCharacterBase>();
		}
		
		if (!ActiveChar || ActiveChar->IsDead() || ActiveChar->ActorHasTag(TEXT("State.Downed")))
		{
			IncapacitatedPlayers++;
		}
	}
	
	if (TotalActivePlayers > 0 && IncapacitatedPlayers >= TotalActivePlayers)
	{
		HandleRaidWipe();
	}
}

void ALumi_SparkGameMode::HandleRaidWipe()
{
	if (ALSGameState* GS = GetGameState<ALSGameState>())
	{
		GS->SetRaidWiped(true);
	}
	if (ULSEventBus* EventBus = ULSEventBus::Get(this))
	{
		EventBus->OnRaidWiped.Broadcast();
	}
	
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("⚠️ 团灭！"));
}
