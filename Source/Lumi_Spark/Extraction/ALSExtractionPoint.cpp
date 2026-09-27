#include "Extraction/ALSExtractionPoint.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Core/LSPlayerController.h"
#include "Core/LSEventBus.h"
#include "Core/Lumi_SparkGameMode.h"
#include "Character/LSCharacterBase.h"
#include "Extraction/ULSCorrosionComponent.h"
#include "Extraction/ULSBackpackComponent.h"

ALSExtractionPoint::ALSExtractionPoint()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // 平时不 Tick，有人踩入才启动

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(RootComponent);
	TriggerBox->SetBoxExtent(FVector(300.0f, 300.0f, 150.0f));
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
	TriggerBox->SetGenerateOverlapEvents(true);

	BasePlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BasePlatformMesh"));
	BasePlatformMesh->SetupAttachment(RootComponent);
	BasePlatformMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ExtractionPointName = FText::FromString(TEXT("紧急避难穿梭井"));
}

void ALSExtractionPoint::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALSExtractionPoint, bIsActive);
}

void ALSExtractionPoint::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ALSExtractionPoint::HandleTriggerBeginOverlap);
		TriggerBox->OnComponentEndOverlap.AddDynamic(this, &ALSExtractionPoint::HandleTriggerEndOverlap);
	}
}

void ALSExtractionPoint::SetExtractionActive(bool bNewActive, const FText& Reason)
{
	if (!HasAuthority()) return;

	if (bIsActive != bNewActive)
	{
		bIsActive = bNewActive;
		OnRep_IsActive();

		// 如果关闭通道，清退所有正在读条的玩家
		if (!bIsActive)
		{
			for (auto& Pair : ExtractingPlayers)
			{
				if (ALSPlayerController* PC = Pair.Key.Get())
				{
					PC->Client_OnExtractionCountdownCanceled();
				}
			}
			ExtractingPlayers.Empty();
			SetActorTickEnabled(false);
		}

		OnExtractionStateChanged.Broadcast(bIsActive, Reason);
	}
}

bool ALSExtractionPoint::IsPlayerExtracting(ALSPlayerController* PC) const
{
	return PC && ExtractingPlayers.Contains(PC);
}

float ALSExtractionPoint::GetPlayerExtractionProgress(ALSPlayerController* PC) const
{
	if (!PC) return 0.0f;
	if (const float* Elapsed = ExtractingPlayers.Find(PC))
	{
		return FMath::Clamp(*Elapsed / FMath::Max(1.0f, ExtractionDuration), 0.0f, 1.0f);
	}
	return 0.0f;
}

ALSPlayerController* ALSExtractionPoint::GetPlayerControllerFromActor(AActor* Actor)
{
	if (!Actor) return nullptr;
	if (ALSCharacterBase* Char = Cast<ALSCharacterBase>(Actor))
	{
		return Cast<ALSPlayerController>(Char->GetController());
	}
	return nullptr;
}

void ALSExtractionPoint::HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !bIsActive) return;

	ALSPlayerController* PC = GetPlayerControllerFromActor(OtherActor);
	if (!PC) return;

	ALSCharacterBase* Char = Cast<ALSCharacterBase>(OtherActor);
	if (!Char || Char->IsDead() || Char->IsDowned()) return;

	// 尚未在该撤离点读条
	if (!ExtractingPlayers.Contains(PC))
	{
		ExtractingPlayers.Add(PC, 0.0f);
		SetActorTickEnabled(true);

		PC->Client_OnExtractionCountdownStarted(ExtractionPointName, ExtractionDuration);
		OnExtractionProgressUpdated.Broadcast(PC, 0.0f, ExtractionDuration);
	}
}

void ALSExtractionPoint::HandleTriggerEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority()) return;

	ALSPlayerController* PC = GetPlayerControllerFromActor(OtherActor);
	if (!PC) return;

	if (ExtractingPlayers.Contains(PC))
	{
		ExtractingPlayers.Remove(PC);
		PC->Client_OnExtractionCountdownCanceled();
		OnExtractionProgressUpdated.Broadcast(PC, 0.0f, 0.0f);

		if (ExtractingPlayers.IsEmpty())
		{
			SetActorTickEnabled(false);
		}
	}
}

void ALSExtractionPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority() || ExtractingPlayers.IsEmpty())
	{
		SetActorTickEnabled(false);
		return;
	}

	TArray<ALSPlayerController*> ToExtract;
	TArray<ALSPlayerController*> ToCancel;

	for (auto& Pair : ExtractingPlayers)
	{
		ALSPlayerController* PC = Pair.Key.Get();
		if (!PC)
		{
			ToCancel.Add(Pair.Key.Get());
			continue;
		}

		APawn* Pawn = PC->GetPawn();
		ALSCharacterBase* Char = Cast<ALSCharacterBase>(Pawn);

		// 中途阵亡或倒地，打断撤离
		if (!Char || Char->IsDead() || Char->IsDowned() || !TriggerBox->IsOverlappingActor(Char))
		{
			ToCancel.Add(PC);
			continue;
		}

		// 累加读条时长
		Pair.Value += DeltaTime;
		const float Remaining = FMath::Max(0.0f, ExtractionDuration - Pair.Value);
		const float Progress = FMath::Clamp(Pair.Value / FMath::Max(1.0f, ExtractionDuration), 0.0f, 1.0f);

		PC->Client_OnExtractionCountdownProgress(Progress, Remaining);
		OnExtractionProgressUpdated.Broadcast(PC, Progress, Remaining);

		// 倒计时结束，加入结算列表
		if (Pair.Value >= ExtractionDuration)
		{
			ToExtract.Add(PC);
		}
	}

	for (ALSPlayerController* CancelPC : ToCancel)
	{
		if (CancelPC)
		{
			CancelPC->Client_OnExtractionCountdownCanceled();
			OnExtractionProgressUpdated.Broadcast(CancelPC, 0.0f, 0.0f);
		}
		ExtractingPlayers.Remove(CancelPC);
	}

	for (ALSPlayerController* ExtractPC : ToExtract)
	{
		ExtractingPlayers.Remove(ExtractPC);
		CompleteExtractionForPlayer(ExtractPC);
	}

	if (ExtractingPlayers.IsEmpty())
	{
		SetActorTickEnabled(false);
	}
}

void ALSExtractionPoint::CompleteExtractionForPlayer(ALSPlayerController* PC)
{
	if (!HasAuthority() || !PC) return;

	// 1. 扣除抗蚀面罩耐久点数（简单-1/中等-1/困难-2/高危-3）
	ULSCorrosionComponent* CorrComp = PC->GetCorrosionComponent();
	const int32 PreDurability = CorrComp ? CorrComp->CurrentMaskDurability : 0;
	if (CorrComp)
	{
		CorrComp->ApplyRaidDurabilityDeduction(ZoneHazard);
	}
	const int32 PostDurability = CorrComp ? CorrComp->CurrentMaskDurability : 0;
	const int32 Deducted = PreDurability - PostDurability;

	// 2. 汇总背包与安全箱战利品
	ULSBackpackComponent* Backpack = PC->GetBackpackComponent();
	const float TotalValue = Backpack ? Backpack->CalculateTotalLootValue() : 0.0f;
	TArray<FLSInventoryItem> AllLoot;
	if (Backpack)
	{
		AllLoot.Append(Backpack->BackpackSlots);
		AllLoot.Append(Backpack->SecureBoxSlots);
	}

	// 3. 构建权威战报
	FLSRaidReport Report;
	Report.Status = ELSExtractionStatus::Extracted;
	Report.HazardLevel = ZoneHazard;
	Report.TotalExtractedValue = TotalValue;
	Report.ExtractedItemCount = AllLoot.Num();
	Report.DurabilityDeducted = Deducted;
	Report.RemainingMaskDurability = PostDurability;
	Report.ExtractedLoot = AllLoot;

	// 4. 事件广播与通知
	OnExtractionCompleted.Broadcast(PC, Report);

	if (ULSEventBus* EventBus = ULSEventBus::Get(this))
	{
		EventBus->OnPlayerExtracted.Broadcast(PC, Report);
	}

	if (ALumi_SparkGameMode* GM = GetWorld()->GetAuthGameMode<ALumi_SparkGameMode>())
	{
		GM->HandlePlayerExtraction(PC, Report);
	}

	PC->Client_OnExtractionSuccess(Report);

	// 5. 若为一次性通道，立即关闭
	if (bSingleUse)
	{
		SetExtractionActive(false, FText::FromString(TEXT("撤离点已满载关闭")));
	}
}

void ALSExtractionPoint::OnRep_IsActive()
{
	// 客户端外观响应（例如点亮或熄灭地面指示灯）
}