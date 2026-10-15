#include "PuzzleResetActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "PuzzleRoomVolume.h"
#include "LumenCharacter.h"
#include "LumenCollisionChannels.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

APuzzleResetActor::APuzzleResetActor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = MeshComp;

	InteractionZone = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionZone"));
	InteractionZone->SetupAttachment(MeshComp);
	InteractionZone->SetSphereRadius(150.f);
	InteractionZone->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionZone->SetCollisionResponseToChannel(ECC_ProjectileTrace, ECR_Ignore);
}

void APuzzleResetActor::BeginPlay()
{
	Super::BeginPlay();
	InteractionZone->OnComponentBeginOverlap.AddDynamic(this, &APuzzleResetActor::OnZoneBegin);
	InteractionZone->OnComponentEndOverlap.AddDynamic(this, &APuzzleResetActor::OnZoneEnd);

	if (!OwningRoom)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PuzzleReset] %s: OwningRoom 미지정"), *GetName());
	}
}

void APuzzleResetActor::OnZoneBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ALumenCharacter* Character = Cast<ALumenCharacter>(OtherActor);
	if (Character && OtherComp == Character->GetCapsuleComponent())
	{
		Character->AddInteractable(this);
	}
}

void APuzzleResetActor::OnZoneEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ALumenCharacter* Character = Cast<ALumenCharacter>(OtherActor);
	if (Character && OtherComp == Character->GetCapsuleComponent())
	{
		Character->RemoveInteractable(this);
	}
}

// ===== ILumenInteractable =====

bool APuzzleResetActor::CanInteract() const
{
	// 이미 풀린 방이면 리셋 불필요 → 프롬프트도 사라짐
	// (쿨타임은 여기 두지 않음 — 넣으면 누를 때마다 프롬프트가 0.5초씩 깜빡임)
	return OwningRoom && !OwningRoom->IsPuzzleSolved();
}

void APuzzleResetActor::Interact(ALumenCharacter* Interactor)
{
	if (!CanInteract()) return;

	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastResetTime < ResetCooldown) return; // 연타 방지

	LastResetTime = Now;
	OwningRoom->ResetMirrors();

	if (ResetSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ResetSound, GetActorLocation());
	}
}

FText APuzzleResetActor::GetInteractPrompt() const
{
	return FText::FromString(TEXT("[E] 거울 초기화"));
}

FVector APuzzleResetActor::GetPromptLocation() const
{
	return FVector(GetActorLocation().X, GetActorLocation().Y, MeshComp->Bounds.GetBox().Max.Z + 30.f);
}