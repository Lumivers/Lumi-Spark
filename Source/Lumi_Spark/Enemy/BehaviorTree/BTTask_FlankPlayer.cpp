#include "BTTask_FlankPlayer.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"
#include "Core/LSTypes.h"

UBTTask_FlankPlayer::UBTTask_FlankPlayer()
{
	NodeName = TEXT("侧翼包抄（FlankPlayer）");
}

EBTNodeResult::Type UBTTask_FlankPlayer::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICon = OwnerComp.GetAIOwner();
	APawn* AIPawn = AICon ? AICon->GetPawn() : nullptr;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!AIPawn || !BB) return EBTNodeResult::Failed;

	AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(LSBlackboardKeys::TargetActor));
	if (!TargetActor) return EBTNodeResult::Failed;
	
	const FVector TargetLoc = TargetActor->GetActorLocation();
	const FVector TargetForward = TargetActor->GetActorForwardVector();
	const FVector DirToAi = (AIPawn->GetActorLocation() - TargetLoc).GetSafeNormal();
	
	//判断ai当前位于目标左侧还是右侧，选择就近方向进行包抄
	const float CrossZ = FVector::CrossProduct(TargetForward, DirToAi).Z;
	const float FlankSign = CrossZ >= 0.0f ? 1.0f : -1.0f; //左侧为正，右侧为负
	
	//沿目标朝向旋转75度，生成侧翼矢量
	const FVector FlankDir = TargetForward.RotateAngleAxis(DesiredFlankAngle * FlankSign, FVector::UpVector);
	const FVector CandidatePoint = TargetLoc + FlankDir * FlankRadius;
	
	//使用导航系统投射到NavMesh有效寻路网格
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys)
	{
		FNavLocation NavLoc;
		if (NavSys->ProjectPointToNavigation(CandidatePoint, NavLoc, FVector(400.0f, 400.0f, 500.0f)))
		{
			BB->SetValueAsVector(LSBlackboardKeys::CoverLocation, NavLoc.Location);
			return EBTNodeResult::Succeeded;
		}
	}
	
	return EBTNodeResult::Failed;
}
