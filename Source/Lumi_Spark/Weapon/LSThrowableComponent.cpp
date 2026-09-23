#include "LSThrowableComponent.h"
#include "Weapon/LSGrenadeBase.h"
#include "Character/LSCharacterBase.h"
#include "Core/LSTypes.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "DrawDebugHelpers.h"

ULSThrowableComponent::ULSThrowableComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false; // 初始休眠，按住 G 才激活 Tick
	SetIsReplicatedByDefault(true);
}

void ULSThrowableComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ULSThrowableComponent, CurrentGrenadeCount);
}

void ULSThrowableComponent::BeginPlay()
{
	Super::BeginPlay();
	SetComponentTickEnabled(false);
}

void ULSThrowableComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsAimingThrow)
	{
		UpdateTrajectoryPrediction();
	}
}

bool ULSThrowableComponent::CanThrow() const
{
	if (CurrentGrenadeCount <= 0) return false;
	if (GetWorld() && (GetWorld()->GetTimeSeconds() - LastThrowTime) < ThrowCooldown) return false;
	return true;
}

void ULSThrowableComponent::AddGrenades(int32 Amount)
{
	if (Amount <= 0) return;
	CurrentGrenadeCount = FMath::Clamp(CurrentGrenadeCount + Amount, 0, MaxGrenadeCount);
	OnRep_CurrentGrenadeCount();
}

void ULSThrowableComponent::StartAimingThrow()
{
	if (!CanThrow()) return;

	bIsAimingThrow = true;
	SetComponentTickEnabled(true); // 激活 Tick 刷新抛物线

	if (ALSCharacterBase* Char = Cast<ALSCharacterBase>(GetOwner()))
	{
		Char->AddGameplayTag(LSTags::TAG_State_AimingThrow);
	}
}

void ULSThrowableComponent::StopAimingThrow(bool bCancel)
{
	if (!bIsAimingThrow) return;

	bIsAimingThrow = false;
	SetComponentTickEnabled(false); // 投掷完成立即关闭 Tick 节省性能

	if (ALSCharacterBase* Char = Cast<ALSCharacterBase>(GetOwner()))
	{
		Char->RemoveGameplayTag(LSTags::TAG_State_AimingThrow);
	}

	if (bCancel)
	{
		OnThrowTrajectoryUpdated.Broadcast(TArray<FVector>(), FHitResult(), false);
		return;
	}

	if (!CanThrow()) return;

	FVector LaunchOrigin;
	FVector LaunchVelocity;
	if (GetLaunchOriginAndVelocity(LaunchOrigin, LaunchVelocity))
	{
		Server_ThrowGrenade(LaunchOrigin, LaunchVelocity);
	}

	OnThrowTrajectoryUpdated.Broadcast(TArray<FVector>(), FHitResult(), false);
}

bool ULSThrowableComponent::GetLaunchOriginAndVelocity(FVector& OutOrigin, FVector& OutVelocity) const
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar) return false;

	FVector CamLoc;
	FRotator CamRot;
	if (APlayerController* PC = Cast<APlayerController>(OwnerChar->GetController()))
	{
		PC->GetPlayerViewPoint(CamLoc, CamRot);
	}
	else
	{
		CamLoc = OwnerChar->GetActorLocation() + FVector(0, 0, 60.0f);
		CamRot = OwnerChar->GetActorRotation();
	}

	OutOrigin = CamLoc + CamRot.Vector() * 70.0f;
	OutVelocity = CamRot.Vector() * ThrowForce;
	return true;
}

void ULSThrowableComponent::UpdateTrajectoryPrediction()
{
	FVector LaunchOrigin;
	FVector LaunchVelocity;
	if (!GetLaunchOriginAndVelocity(LaunchOrigin, LaunchVelocity)) return;

	FPredictProjectilePathParams Params;
	Params.StartLocation = LaunchOrigin;
	Params.LaunchVelocity = LaunchVelocity;
	Params.bTraceWithCollision = true;
	Params.ProjectileRadius = 14.0f;
	Params.MaxSimTime = MaxSimTime;
	Params.bTraceWithChannel = true;
	Params.TraceChannel = PredictionTraceChannel;
	Params.ActorsToIgnore.Add(GetOwner());
	Params.SimFrequency = 20.0f;
	Params.bTraceComplex = false;

	FPredictProjectilePathResult Result;
	const bool bHit = UGameplayStatics::PredictProjectilePath(this, Params, Result);

	TArray<FVector> PathPoints;
	PathPoints.Reserve(Result.PathData.Num());
	for (const FPredictProjectilePathPointData& Point : Result.PathData)
	{
		PathPoints.Add(Point.Location);
	}

	// 实时绘制一帧时长的绿色平滑抛物线
	for (int32 i = 0; i < PathPoints.Num() - 1; ++i)
	{
		DrawDebugLine(GetWorld(), PathPoints[i], PathPoints[i + 1], FColor::Emerald, false, -1.0f, 0, 2.5f);
	}

	// 落地落点瞄准圈
	if (bHit)
	{
		const FVector HitPoint = Result.HitResult.ImpactPoint;
		const FVector HitNormal = Result.HitResult.ImpactNormal;
		DrawDebugCircle(GetWorld(), HitPoint + HitNormal * 2.0f, 50.0f, 16, FColor::Cyan, false, -1.0f, 0, 2.0f, HitNormal, FVector::CrossProduct(HitNormal, FVector::ForwardVector).GetSafeNormal());
		DrawDebugPoint(GetWorld(), HitPoint, 8.0f, FColor::Red, false, -1.0f, 0);
	}

	OnThrowTrajectoryUpdated.Broadcast(PathPoints, Result.HitResult, bHit);
}

TSubclassOf<ALSGrenadeBase> ULSThrowableComponent::ResolveGrenadeClass() const
{
	if (ALSCharacterBase* OwnerChar = Cast<ALSCharacterBase>(GetOwner()))
	{
		const FGameplayTag ElemTag = OwnerChar->GetCharacterElementTag();
		if (ElemTag == LSTags::TAG_Element_Pyro) return ALSGrenade_Pyro::StaticClass();
		if (ElemTag == LSTags::TAG_Element_Hydro) return ALSGrenade_Hydro::StaticClass();
		if (ElemTag == LSTags::TAG_Element_Cryo) return ALSGrenade_Cryo::StaticClass();
		if (ElemTag == LSTags::TAG_Element_Electro) return ALSGrenade_Electro::StaticClass();
		if (ElemTag == LSTags::TAG_Element_Dendro) return ALSGrenade_Dendro::StaticClass();
		if (ElemTag == LSTags::TAG_Element_Anemo) return ALSGrenade_Anemo::StaticClass();
	}
	if (DefaultGrenadeClass)
	 	{
	 		return DefaultGrenadeClass;
	 	}
	return ALSGrenade_Pyro::StaticClass();
}

void ULSThrowableComponent::Server_ThrowGrenade_Implementation(FVector LaunchOrigin, FVector LaunchVelocity)
{
	if (!CanThrow()) return;

	CurrentGrenadeCount = FMath::Max(0, CurrentGrenadeCount - 1);
	LastThrowTime = GetWorld()->GetTimeSeconds();
	OnRep_CurrentGrenadeCount();

	TSubclassOf<ALSGrenadeBase> GrenadeClassToSpawn = ResolveGrenadeClass();
	if (!GrenadeClassToSpawn) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = Cast<APawn>(GetOwner());
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FRotator SpawnRot = LaunchVelocity.Rotation();
	if (ALSGrenadeBase* Grenade = GetWorld()->SpawnActor<ALSGrenadeBase>(GrenadeClassToSpawn, LaunchOrigin, SpawnRot, SpawnParams))
	{
		if (UProjectileMovementComponent* ProjComp = Grenade->GetProjectileMovement())
		{
			ProjComp->Velocity = LaunchVelocity;
		}
	}
}

void ULSThrowableComponent::OnRep_CurrentGrenadeCount()
{
	OnGrenadeCountChanged.Broadcast(CurrentGrenadeCount, MaxGrenadeCount);
}