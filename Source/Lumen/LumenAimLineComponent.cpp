//// LumenAimLineComponent.cpp

#include "LumenAimLineComponent.h"
#include "Lumen/Public/WeaponDataAsset.h" // 실제 프로젝트의 include 경로에 맞춰 조정할 것
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Actor.h"

ULumenAimLineComponent::ULumenAimLineComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void ULumenAimLineComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner || !Owner->GetRootComponent())
	{
		UE_LOG(LogTemp, Warning, TEXT("ULumenAimLineComponent: Owner 또는 RootComponent가 없어 조준선을 생성하지 않음"));
		return;
	}

	MeshComp = NewObject<UProceduralMeshComponent>(Owner, TEXT("AimLineMesh"));
	MeshComp->RegisterComponent();
	MeshComp->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	MeshComp->SetRelativeLocation(FVector(0.f, 0.f, HeightOffset));
	MeshComp->SetCastShadow(false);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (BaseMaterial)
	{
		DynamicMat = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		MeshComp->SetMaterial(0, DynamicMat);
	}

	RebuildMesh();
}

void ULumenAimLineComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!MeshComp) return;

	FVector FlatDir = AimDirection;
	FlatDir.Z = 0.f;
	if (!FlatDir.IsNearlyZero())
	{
		FlatDir.Normalize();
		MeshComp->SetWorldRotation(FlatDir.Rotation());
	}
}

void ULumenAimLineComponent::SetAimDirection(const FVector& WorldAimDirection)
{
	AimDirection = WorldAimDirection;
}

void ULumenAimLineComponent::SetWeaponData(UWeaponDataAsset* NewWeapon)
{
	if (CurrentWeapon == NewWeapon) return;
	CurrentWeapon = NewWeapon;
	RebuildMesh();
}

void ULumenAimLineComponent::SetVisible(bool bVisible)
{
	if (MeshComp)
	{
		MeshComp->SetVisibility(bVisible);
	}
}

void ULumenAimLineComponent::GetShapeFromWeapon(float& OutLength, float& OutHalfWidth, FLinearColor& OutColor) const
{
	if (!CurrentWeapon)
	{
		OutLength = FallbackLength;
		OutHalfWidth = FallbackHalfWidth;
		OutColor = FallbackColor;
		return;
	}

	// 5-2-1 결정: 초록(LineTrace)은 TraceRange가 실제 판정 사거리라 하드 캡이 정상 동작.
	// 빨강/파랑(Projectile)은 감쇠 종료 지점(FalloffEndRange)이 "실질적 사거리" — 그 이후로도
	// 투사체 자체는 계속 날아가지만(반사 퍼즐과의 충돌 방지), 데미지가 MinDamageMultiplier로
	// 떨어지므로 조준선은 여기까지만 표시해 실제 위력 범위를 정직하게 보여줌.
	OutLength = (CurrentWeapon->FireMode == EWeaponFireMode::LineTrace)
		? CurrentWeapon->TraceRange
		: CurrentWeapon->FalloffEndRange;

	const float HalfAngleRad = FMath::DegreesToRadians(CurrentWeapon->TelegraphWidthDegrees * 0.5f);
	OutHalfWidth = OutLength * FMath::Tan(HalfAngleRad);

	OutColor = CurrentWeapon->WeaponColor_TEMP;
}

void ULumenAimLineComponent::RebuildMesh()
{
	if (!MeshComp) return;

	float Length, HalfWidth;
	FLinearColor Color;
	GetShapeFromWeapon(Length, HalfWidth, Color);

	TArray<FVector> Vertices;
	Vertices.Add(FVector(0.f, 0.f, 0.f));            // apex
	Vertices.Add(FVector(Length, -HalfWidth, 0.f));  // base left
	Vertices.Add(FVector(Length, HalfWidth, 0.f));   // base right

	TArray<int32> Triangles = { 0, 1, 2 };
	TArray<FVector> Normals = { FVector::UpVector, FVector::UpVector, FVector::UpVector };
	TArray<FVector2D> UVs = { FVector2D(0.f, 0.f), FVector2D(1.f, 0.f), FVector2D(1.f, 1.f) };
	TArray<FLinearColor> VertexColors = { Color, Color, Color };
	TArray<FProcMeshTangent> Tangents;

	MeshComp->ClearAllMeshSections();
	MeshComp->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, false);

	if (DynamicMat)
	{
		DynamicMat->SetVectorParameterValue(TEXT("TintColor"), Color);
	}
}