#include "LumenDoorActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "LumenCharacter.h"
#include "LumenCollisionChannels.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

ALumenDoorActor::ALumenDoorActor()
{
	PrimaryActorTick.bCanEverTick = true;

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	RootComponent = DoorMesh;
	DoorMesh->SetCollisionProfileName(TEXT("BlockAllDynamic")); // 닫혀있을 때 캐릭터/발사체 막음

	InteractionZone = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionZone"));
	InteractionZone->SetupAttachment(DoorMesh);
	InteractionZone->SetSphereRadius(150.f);
	InteractionZone->SetCollisionProfileName(TEXT("Trigger"));
	// Trigger 프리셋은 커스텀 채널 ProjectileTrace를 Block으로 물려받음
	// → 그대로 두면 문 주변 반경 안에서 발사체가 멈추고 조준선도 둥글게 잘림
	InteractionZone->SetCollisionResponseToChannel(ECC_ProjectileTrace, ECR_Ignore);

	// ClosedLocation은 BeginPlay에서 캡처 — 생성자 시점엔 레벨 배치 트랜스폼이 적용되기 전이라 (0,0,0)이 나옴
}

void ALumenDoorActor::BeginPlay()
{
	Super::BeginPlay();
	ClosedLocation = GetActorLocation();
	ClosedTopZ = DoorMesh->Bounds.GetBox().Max.Z;

	// bInteractable일 때만 오버랩 감지 (자동 클리어 문은 상호작용 대상이 아님)
	if (bInteractable)
	{
		InteractionZone->OnComponentBeginOverlap.AddDynamic(this, &ALumenDoorActor::OnInteractionZoneBeginOverlap);
		InteractionZone->OnComponentEndOverlap.AddDynamic(this, &ALumenDoorActor::OnInteractionZoneEndOverlap);
	}
}

void ALumenDoorActor::OnInteractionZoneBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ALumenCharacter* Player = Cast<ALumenCharacter>(OtherActor);
	if (Player && OtherComp == Player->GetCapsuleComponent())
	{
		Player->AddInteractable(this);
	}
}

void ALumenDoorActor::OnInteractionZoneEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ALumenCharacter* Player = Cast<ALumenCharacter>(OtherActor);
	if (Player && OtherComp == Player->GetCapsuleComponent())
	{
		Player->RemoveInteractable(this);
	}
}

// ===== ILumenInteractable =====

void ALumenDoorActor::Interact(ALumenCharacter* Interactor)
{
	ToggleInteract();
}

bool ALumenDoorActor::CanInteract() const
{
	// 움직이는 중에는 연타 방지 — 이동이 끝나면 프롬프트가 "열기/닫기"로 바뀌어 다시 뜸
	return bInteractable && !bIsMoving;
}

FText ALumenDoorActor::GetInteractPrompt() const
{
	return FText::FromString(bIsOpen ? TEXT("[E] 닫기") : TEXT("[E] 열기"));
}

FVector ALumenDoorActor::GetPromptLocation() const
{
	return FVector(ClosedLocation.X, ClosedLocation.Y, ClosedTopZ + 30.f);
}

// ===== 여닫기 =====

void ALumenDoorActor::Open()
{
	if (bIsOpen)
	{
		return;
	}

	bIsOpen = true;
	bIsMoving = true;
	MoveElapsed = 0.f;
	MoveStartLocation = GetActorLocation();
	TargetLocation = ClosedLocation + FVector(0.f, 0.f, OpenHeight);

	if (OpenSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, OpenSound, ClosedLocation);
	}

	// 열리는 동안엔 막지 않도록 콜리전을 미리 꺼둠 (슬라이드 중 캐릭터가 통과 못 하는 것 방지)
	DoorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ALumenDoorActor::Close()
{
	if (!bIsOpen)
	{
		return;
	}

	bIsOpen = false;
	bIsMoving = true;
	MoveElapsed = 0.f;
	MoveStartLocation = GetActorLocation();
	TargetLocation = ClosedLocation;

	if (CloseSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, CloseSound, ClosedLocation);
	}
}

void ALumenDoorActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsMoving)
	{
		return;
	}

	MoveElapsed += DeltaTime;
	const float Alpha = FMath::Clamp(MoveElapsed / FMath::Max(OpenDuration, 0.01f), 0.f, 1.f);

	SetActorLocation(FMath::Lerp(MoveStartLocation, TargetLocation, Alpha));

	if (Alpha >= 1.f)
	{
		bIsMoving = false;

		// 다 닫혔을 때만 다시 콜리전 켜기 (열린 상태에서는 계속 비활성 유지)
		if (!bIsOpen)
		{
			DoorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
	}
}