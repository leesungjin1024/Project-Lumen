// LumenDoorActor.cpp

#include "LumenDoorActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "LumenCharacter.h"

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

	// ClosedLocation은 여기서 캡처하지 않음 — 생성자 시점엔 레벨 배치 트랜스폼이 아직 적용되기 전이라
	// GetActorLocation()이 항상 (0,0,0)을 반환함 (BeginPlay에서 캡처)
}

void ALumenDoorActor::BeginPlay()
{
	Super::BeginPlay();
	ClosedLocation = GetActorLocation();

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
	if (ALumenCharacter* Player = Cast<ALumenCharacter>(OtherActor))
	{
		// 문이 여러 개 겹쳐 있을 수 있으므로 "목록에 추가" 방식 사용 (InteractableDoor_Patch.md 참고)
		Player->AddInteractableDoor(this);
	}
}

void ALumenDoorActor::OnInteractionZoneEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (ALumenCharacter* Player = Cast<ALumenCharacter>(OtherActor))
	{
		Player->RemoveInteractableDoor(this);
	}
}

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