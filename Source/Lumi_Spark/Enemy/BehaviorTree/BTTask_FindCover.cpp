#include "BTTask_FindCover.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/ALSCoverPoint.h"
#include "Core/LSTypes.h"
#include "Kismet/GameplayStatics.h"

UBTTask_FindCover::UBTTask_FindCover()
{
	NodeName = TEXT("寻找战术掩体（FindCover）");
}

EBTNodeResult::Type UBTTask_FindCover::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	APawn* AIPawn = AICon ? AICon->GetPawn() : nullptr;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AIPawn || !BB) return EBTNodeResult::Failed;
	
	//获取威胁位置（优先目标实体，其次最后已知目标）
	AActor* ThreatActor = Cast<AActor>(BB->GetValueAsObject(LSBlackboardKeys::TargetActor));
	const FVector ThreatLoc = ThreatActor ? ThreatActor->GetActorLocation() : BB->GetValueAsVector(LSBlackboardKeys::LastKnownLocation);
	if (ThreatLoc.IsZero()) return EBTNodeResult::Failed;
	
	// 寻找关卡内所有掩体
	TArray<AActor*> CoverPoints;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AALSCoverPoint::StaticClass(), CoverPoints);
	
	AALSCoverPoint* BestCover = nullptr;
	float BestDistSq = MAX_FLT;
	const FVector PawnLoc = AIPawn->GetActorLocation();
	
	for (AActor* Actor : CoverPoints)
	{
		AALSCoverPoint* Cover = Cast<AALSCoverPoint>(Actor);
		if (!Cover || Cover->IsOccupied()) continue;
		
		const float DistSq = FVector::DistSquared(PawnLoc, Cover->GetActorLocation());
		if (DistSq > FMath::Square(MaxSearchRadius)) continue;
		
		//判定该掩体能否阻断来自威胁方向的射击视线
		if (Cover->IsValidAgainst(ThreatLoc))
		{
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestCover = Cover;
			}
		}
	}
	
	if (BestCover)
	{
		BestCover->SetOccupied(true, AIPawn);
		BB->SetValueAsObject(LSBlackboardKeys::CoverLocation, BestCover->GetActorLocation());
		return EBTNodeResult::Succeeded;
	}
	
	return EBTNodeResult::Failed;
}