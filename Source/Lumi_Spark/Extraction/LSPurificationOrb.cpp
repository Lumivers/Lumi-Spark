#include "Extraction/LSPurificationOrb.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Character/LSCharacterBase.h"
#include "Core/LSPlayerController.h"
#include "Extraction/ULSCorrosionComponent.h"

ALSPurificationOrb::ALSPurificationOrb()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->InitSphereRadius(75.0f);
	CollisionSphere->SetCollisionProfileName(TEXT("Trigger"));
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	OrbMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OrbMesh"));
	OrbMesh->SetupAttachment(RootComponent);
	OrbMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RotatingMovement = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("RotatingMovement"));
	RotatingMovement->RotationRate = FRotator(0.0f, 90.0f, 0.0f);
}

void ALSPurificationOrb::BeginPlay()
{
	Super::BeginPlay();

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ALSPurificationOrb::HandleSphereBeginOverlap);
}

void ALSPurificationOrb::HandleSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bIsCollected || !OtherActor) return;

	if (ALSCharacterBase* Character = Cast<ALSCharacterBase>(OtherActor))
	{
		if (ALSPlayerController* PC = Cast<ALSPlayerController>(Character->GetController()))
		{
			if (ULSCorrosionComponent* CorrosionComp = PC->GetCorrosionComponent())
			{
				bIsCollected = true;

				// 扣减侵蚀度
				CorrosionComp->ReduceCorrosion(CleanseAmount);

				// 播放拾取视听反馈
				if (PickupSound)
				{
					UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
				}
				if (PickupEmitter)
				{
					UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), PickupEmitter, GetActorLocation());
				}

				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green,
						FString::Printf(TEXT("✨ [地脉净化] 拾取净化光球，侵蚀度已回退 -%.0f%%！"), CleanseAmount));
				}

				Destroy();
			}
		}
	}
}
