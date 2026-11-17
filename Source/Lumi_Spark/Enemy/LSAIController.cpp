#include "LSAIController.h"
#include "LSEnemyBase.h"
#include "LSEnemyDataAsset.h"
#include "Combat/LSThreatComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Damage.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"

ALSAIController::ALSAIController()
{
	PrimaryActorTick.bCanEverTick = false;

	// 1. 初始化感知组件
	AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComp"));
	SetPerceptionComponent(*AIPerceptionComp);

	// 2. 配置视觉 Sense
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = SightAngle;
	SightConfig->SetMaxAge(5.0f);
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	AIPerceptionComp->ConfigureSense(*SightConfig);

	// 3. 配置听觉 Sense (枪声)
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = HearingRange;
	HearingConfig->SetMaxAge(3.0f);
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	AIPerceptionComp->ConfigureSense(*HearingConfig);

	// 4. 配置受击伤害 Sense
	DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageConfig"));
	DamageConfig->SetMaxAge(5.0f);
	AIPerceptionComp->ConfigureSense(*DamageConfig);

	AIPerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());
}

void ALSAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ControlledEnemy = Cast<ALSEnemyBase>(InPawn);
	if (!ControlledEnemy.IsValid()) return;

	// 绑定感知更新回调
	if (AIPerceptionComp)
	{
		AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &ALSAIController::HandleTargetPerceptionUpdated);
	}

	// 启动数据资产中挂接的行为树
	if (ULSEnemyDataAsset* EnemyData = ControlledEnemy->GetEnemyDataAsset())
	{
		if (EnemyData->BehaviorTree)
		{
			RunBehaviorTree(EnemyData->BehaviorTree);
			if (Blackboard)
			{
				Blackboard->SetValueAsName(LSBlackboardKeys::AIState, FName(TEXT("Idle")));
			}
		}
	}
}

void ALSAIController::OnUnPossess()
{
	if (AIPerceptionComp)
	{
		AIPerceptionComp->OnTargetPerceptionUpdated.RemoveDynamic(this, &ALSAIController::HandleTargetPerceptionUpdated);
	}

	GetWorldTimerManager().ClearTimer(LoseSightTimerHandle);
	Super::OnUnPossess();
}

void ALSAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor) return;

	if (Stimulus.WasSuccessfullySensed())
	{
		// 清除脱失记忆计时
		GetWorldTimerManager().ClearTimer(LoseSightTimerHandle);

		// 记录感知最后已知坐标（若有黑板）
		if (Blackboard)
		{
			Blackboard->SetValueAsVector(LSBlackboardKeys::LastKnownLocation, Stimulus.StimulusLocation);
		}

		// 若之前没有目标，或被攻击/看到新威胁，重新评定最高仇恨目标并立即追击
		UpdateBestTarget();

		if (ControlledEnemy.IsValid())
		{
			ControlledEnemy->SetCombatMovementSpeed(true);
		}
	}
	else
	{
		// 目标丢失视野，启动 5.0 秒记忆倒计时
		if (Actor == GetTargetActor())
		{
			GetWorldTimerManager().SetTimer(LoseSightTimerHandle, this, &ALSAIController::HandleLoseSightExpired, LoseSightDuration, false);
		}
	}
}

void ALSAIController::UpdateBestTarget()
{
	AActor* BestTarget = nullptr;

	// 优先查询敌人的仇恨表组件 (ThreatComponent) 获取最高威胁目标
	if (ControlledEnemy.IsValid())
	{
		if (ULSThreatComponent* ThreatComp = ControlledEnemy->GetThreatComponent())
		{
			BestTarget = ThreatComp->GetHighestThreatTarget();
		}
	}

	// 若仇恨表无目标，则从感知已看到的敌人列表中挑选最近目标
	if (!BestTarget && AIPerceptionComp && SightConfig)
	{
		TArray<AActor*> PerceivedActors;
		AIPerceptionComp->GetCurrentlyPerceivedActors(SightConfig->GetSenseImplementation(), PerceivedActors);

		float ClosestDistSq = MAX_FLT;
		const FVector MyLoc = ControlledEnemy.IsValid() ? ControlledEnemy->GetActorLocation() : FVector::ZeroVector;

		for (AActor* Perceived : PerceivedActors)
		{
			if (Perceived && Perceived != GetPawn())
			{
				const float DistSq = FVector::DistSquared(MyLoc, Perceived->GetActorLocation());
				if (DistSq < ClosestDistSq)
				{
					ClosestDistSq = DistSq;
					BestTarget = Perceived;
				}
			}
		}
	}

	if (BestTarget)
	{
		// 1. 视线焦点对准目标（转头/举枪朝向玩家）
		SetFocus(BestTarget);

		// 2. 写入黑板（若黑板已初始化）
		if (Blackboard)
		{
			Blackboard->SetValueAsObject(LSBlackboardKeys::TargetActor, BestTarget);
			Blackboard->SetValueAsName(LSBlackboardKeys::AIState, FName(TEXT("Combat")));

			if (ControlledEnemy.IsValid())
			{
				const float Dist = FVector::Dist(ControlledEnemy->GetActorLocation(), BestTarget->GetActorLocation());
				Blackboard->SetValueAsFloat(LSBlackboardKeys::DistanceToTarget, Dist);
			}
		}

		// 3. 驱动敌人切换至战斗跑步移速
		if (ControlledEnemy.IsValid())
		{
			ControlledEnemy->SetCombatMovementSpeed(true);
		}

		// 4. 原生战术行为兜底：若未挂接/未运行行为树，直接驱动 MoveToActor 贴身逼近（近战 120cm 距离）
		if (!BrainComponent || !BrainComponent->IsRunning())
		{
			MoveToActor(BestTarget, 120.0f);
		}
	}
}

void ALSAIController::HandleLoseSightExpired()
{
	ClearFocus(EAIFocusPriority::Gameplay);
	StopMovement();

	if (Blackboard)
	{
		// 5 秒过去依然未看到玩家，放弃锁定并降级为警戒搜寻
		Blackboard->ClearValue(LSBlackboardKeys::TargetActor);
		Blackboard->SetValueAsName(LSBlackboardKeys::AIState, FName(TEXT("Alert")));
	}

	if (ControlledEnemy.IsValid())
	{
		ControlledEnemy->SetCombatMovementSpeed(false);
	}
}

AActor* ALSAIController::GetTargetActor() const
{
	if (Blackboard)
	{
		return Cast<AActor>(Blackboard->GetValueAsObject(LSBlackboardKeys::TargetActor));
	}
	return GetFocusActor();
}