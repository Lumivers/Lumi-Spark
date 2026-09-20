// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Lumi_SparkGameMode.generated.h"

class ALSPlayerController;

UCLASS(minimalapi)
class ALumi_SparkGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALumi_SparkGameMode();
	
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	
	// 权威检查所有玩家状态，若所有玩家均处于倒地或者死亡，判定团灭
	UFUNCTION(BlueprintCallable, Category = "GameMode|Raid")
	void CheckRaidWipeCondition();
	
protected:
	// 触发团灭结算流程
	virtual void HandleRaidWipe();
};