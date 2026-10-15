#include "TestActor.h"

ATestActor::ATestActor()
{
	PrimaryActorTick.bCanEverTick = true;
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
}

void ATestActor::BeginPlay()
{
	Super::BeginPlay();
	
}

void ATestActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}



