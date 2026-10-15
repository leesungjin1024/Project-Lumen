// ALumenBeamSegment 구현 — 자세한 설명은 LumenBeamSegment.h

#include "LumenBeamSegment.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

ALumenBeamSegment::ALumenBeamSegment()
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	RootComponent = MeshComp;
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 보이기만 함 — 판정은 라인트레이스가 이미 끝냄
	MeshComp->SetCastShadow(false);
}

void ALumenBeamSegment::InitSegment(const FVector& Start, const FVector& End, const FLinearColor& Color)
{
	const FVector Delta = End - Start;
	SegmentLength = Delta.Size();

	if (SegmentLength < 1.f)
	{
		Destroy(); // 길이가 거의 0인 세그먼트(벽에 붙어서 쏜 경우 등)는 그릴 필요 없음
		return;
	}

	// 엔진 Cylinder는 피벗이 중심, 높이 방향이 Z축 → 중점에 놓고 Z축을 진행 방향에 맞춤
	const FVector Mid = Start + Delta * 0.5f;
	const FRotator Rot = FRotationMatrix::MakeFromZ(Delta / SegmentLength).Rotator();
	SetActorLocationAndRotation(Mid, Rot);

	ApplyScale(1.f);

	if (!MeshMID)
	{
		MeshMID = MeshComp->CreateDynamicMaterialInstance(0);
	}
	if (MeshMID)
	{
		MeshMID->SetVectorParameterValue(TEXT("Color"), Color); // M_Projectile과 같은 파라미터 이름
	}

	Elapsed = 0.f;
	SetLifeSpan(LifeTime + 0.5f); // 혹시 Tick이 안 돌아도 남지 않게 하는 안전장치
}

void ALumenBeamSegment::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Elapsed += DeltaTime;
	const float Alpha = FMath::Clamp(Elapsed / LifeTime, 0.f, 1.f);

	// 길이는 그대로 두고 두께만 줄임 — 경로(반사각)는 사라질 때까지 정확히 보임
	ApplyScale(1.f - Alpha);

	if (Alpha >= 1.f)
	{
		Destroy();
	}
}

void ALumenBeamSegment::ApplyScale(float WidthScale)
{
	const float W = (BeamWidth * WidthScale) / MeshBaseSize;
	const float L = SegmentLength / MeshBaseSize;
	MeshComp->SetRelativeScale3D(FVector(W, W, L));
}
