// AMirrorActor 구현 — 자세한 설명은 MirrorActor.h
#include "MirrorActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "LumenCharacter.h"
#include "LumenCollisionChannels.h"

AMirrorActor::AMirrorActor()
{
	CollisionComp = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComp"));
	// 스치는(그레이징) 각도에서도 맞도록 시각 메시보다 콜리전을 살짝 두껍게
	CollisionComp->InitBoxExtent(FVector(10.f, 100.f, 150.f));

	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComp->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	CollisionComp->SetCollisionObjectType(ECC_Reflective);
	CollisionComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore); // AI LOS 트레이스는 그대로 통과
	CollisionComp->SetCollisionResponseToChannel(ECC_ProjectileTrace, ECR_Block); // Visibility를 무시하므로 발사체 판정은 반드시 이 채널로

	RootComponent = CollisionComp;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(RootComponent);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 상호작용 범위는 반사 판정 박스보다 넉넉하게 - 캐릭터가 가까이 오면 감지
	InteractionZone = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionZone"));
	InteractionZone->SetupAttachment(RootComponent);
	InteractionZone->SetSphereRadius(250.f);
	InteractionZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionZone->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionZone->SetCollisionResponseToChannel(ECC_ProjectileTrace, ECR_Ignore); // 발사체는 이 구체를 완전히 무시

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // 회전 중 / E 홀드 판정 중에만 켬
}

void AMirrorActor::BeginPlay()
{
	Super::BeginPlay();

	InitialRotation = GetActorRotation();
	TargetRotation = InitialRotation;

	if (bIsRotatable)
	{
		InteractionZone->OnComponentBeginOverlap.AddDynamic(this, &AMirrorActor::OnInteractionZoneBeginOverlap);
		InteractionZone->OnComponentEndOverlap.AddDynamic(this, &AMirrorActor::OnInteractionZoneEndOverlap);
	}
}

void AMirrorActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// ── E 홀드 판정: 임계 시간이 차면 손 떼기 전에 바로 역회전 ──
	if (bPressPending && GetWorld()->GetTimeSeconds() - PressStartTime >= MirrorReverseHoldTime)
	{
		bPressPending = false;
		RotateStep(-1);
	}

	// ── 회전 보간 ──
	if (bIsRotating)
	{
		const FRotator NewRot = FMath::RInterpConstantTo(
			GetActorRotation(), TargetRotation, DeltaTime, RotationSpeedDegrees);

		if (NewRot.Equals(TargetRotation, 0.1f))
		{
			FinishRotation();
		}
		else
		{
			SetActorRotation(NewRot);
		}
	}

	if (!bIsRotating && !bPressPending)
	{
		SetActorTickEnabled(false);
	}
}

FVector AMirrorActor::GetReflectionNormal(const FHitResult& Hit) const
{
	FVector Normal = GetActorForwardVector();
	Normal.Z = 0.f;              // 반사는 수평면에서만 — Z=0 고정
	return Normal.GetSafeNormal();
}

void AMirrorActor::RotateStep(int32 Direction)
{
	if (!bIsRotatable || bIsRotating) return;   // 회전 중 입력 잠금

	FRotator NewTarget = TargetRotation;
	NewTarget.Yaw = FRotator::NormalizeAxis(NewTarget.Yaw + RotationStepDegrees * Direction);
	StartRotatingTo(NewTarget);
	// Yaw만 바꿈 - Pitch/Roll은 건드리지 않아 반사가 수평으로만 유지됨
}

// ===== ILumenInteractable =====

void AMirrorActor::Interact(ALumenCharacter* Interactor)
{
	// 누른 순간엔 기록만 — 짧게 떼면 정방향(InteractReleased), 길게 누르면 역방향(Tick)
	if (!bIsRotatable || bIsRotating) return;

	bPressPending = true;
	PressStartTime = GetWorld()->GetTimeSeconds();
	SetActorTickEnabled(true);
}

void AMirrorActor::InteractReleased(ALumenCharacter* Interactor)
{
	if (bPressPending)
	{
		bPressPending = false;
		RotateStep(+1);   // 홀드 임계 전에 뗐다 = 짧게 누름 = 정방향
	}
}

void AMirrorActor::InteractCanceled()
{
	bPressPending = false;
}

bool AMirrorActor::CanInteract() const
{
	// 회전 중에도 true — false로 하면 회전할 때마다 프롬프트가 깜빡임 (회전 중 입력은 Interact에서 무시)
	return bIsRotatable;
}

FText AMirrorActor::GetInteractPrompt() const
{
	return FText::FromString(TEXT("[E] 회전   (길게: 반대로)"));
}

FVector AMirrorActor::GetPromptLocation() const
{
	// 거울 판정 박스 위쪽
	return GetActorLocation() + FVector(0.f, 0.f, CollisionComp->GetScaledBoxExtent().Z + 30.f);
}

// ===== 오버랩 =====

void AMirrorActor::OnInteractionZoneBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ALumenCharacter* Character = Cast<ALumenCharacter>(OtherActor);
	if (Character && OtherComp == Character->GetCapsuleComponent())
	{
		Character->AddInteractable(this);
	}
}

void AMirrorActor::OnInteractionZoneEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ALumenCharacter* Character = Cast<ALumenCharacter>(OtherActor);
	if (Character && OtherComp == Character->GetCapsuleComponent())
	{
		Character->RemoveInteractable(this);
		bPressPending = false; // 누른 채 범위를 벗어나면 홀드 취소
	}
}

// ===== 회전 =====

void AMirrorActor::ResetToInitialRotation()
{
	if (!bIsRotatable) return;
	StartRotatingTo(InitialRotation);   // 회전 중이어도 목표를 덮어씀
}

void AMirrorActor::StartRotatingTo(const FRotator& NewTarget)
{
	TargetRotation = NewTarget;
	if (GetActorRotation().Equals(TargetRotation, 0.1f))
	{
		FinishRotation();
		return;
	}
	bIsRotating = true;
	SetActorTickEnabled(true);
}

void AMirrorActor::FinishRotation()
{
	SetActorRotation(TargetRotation);   // 오차 없이 정확한 각도로 스냅
	bIsRotating = false;
	SetActorTickEnabled(bPressPending); // 홀드 판정 중이면 Tick 유지
}