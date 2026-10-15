// ULumenAimLineComponent 구현 — 자세한 설명은 LumenAimLineComponent.h

#include "LumenAimLineComponent.h"
#include "WeaponDataAsset.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Actor.h"
#include "LumenCollisionChannels.h"

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
	// 액터/캡슐 Scale 무시 → 조준선 길이 = 무기 사거리(cm) 그대로 (포탑을 키워도 선이 늘어나지 않음)
	MeshComp->SetUsingAbsoluteScale(true);
	MeshComp->SetRelativeLocation(FVector(0.f, 0.f, HeightOffset));
	ApplyOrigin();   // BeginPlay 전에 SetOriginLocation이 먼저 불렸을 경우
	MeshComp->SetCastShadow(false);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (BaseMaterial)
	{
		DynamicMat = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	}

	RebuildMesh();
}

void ULumenAimLineComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!MeshComp) return;

	FVector FlatDir = AimDirection;
	FlatDir.Z = 0.f;
	if (FlatDir.IsNearlyZero())
	{
		return;
	}
	FlatDir.Normalize();
	ApplyOrigin();
	MeshComp->SetWorldRotation(FlatDir.Rotation());

	// 숨겨져 있으면 트레이스/메시 갱신 생략 (방패 중, 평소의 적)
	if (!MeshComp->IsVisible())
	{
		return;
	}

	UpdateRayLengths(FlatDir);

	if (bTelegraphMode)
	{
		// 텔레그래프 시간 진행은 이 컴포넌트가 직접 담당 (적 캐릭터 Tick과 무관)
		// Duration을 넘어가면 번쩍 상태로 유지 (BT가 Hide할 때까지)
		TelegraphElapsed = FMath::Min(TelegraphElapsed + DeltaTime, TelegraphTotal);
		UpdateTelegraphVisual();
	}
	else
	{
		WriteFan(0, -1.f, CachedColor, false);
	}
}

void ULumenAimLineComponent::SetAimDirection(const FVector& WorldAimDirection)
{
	AimDirection = WorldAimDirection;
}

void ULumenAimLineComponent::SetOriginLocation(const FVector& WorldLocation)
{
	OriginLocation = WorldLocation;
	bHasOrigin = true;
	ApplyOrigin();
}

void ULumenAimLineComponent::ApplyOrigin()
{
	if (!MeshComp || !bHasOrigin)
	{
		return;
	}

	FVector Origin = OriginLocation;

	// 벽에 딱 붙으면 총구 소켓이 벽 너머로 나가 조준선이 벽 반대편에서 시작될 수 있음 —
	// 몸 중심(총구 높이) → 총구 사이에 벽/거울/문이 있으면 그 벽 앞에서 시작.
	// 발사체/레이저 스폰 위치 보정(몸 중심 → 총구 사전 트레이스)과 같은 규칙
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (Owner && World)
	{
		FVector Center = Owner->GetActorLocation();
		Center.Z = Origin.Z;

		if (!Center.Equals(Origin, 0.1f))
		{
			FCollisionQueryParams Params(SCENE_QUERY_STAT(AimLineOriginTrace), false, Owner);
			TArray<AActor*> AttachedActors;
			Owner->GetAttachedActors(AttachedActors);   // 내 방패 등
			Params.AddIgnoredActors(AttachedActors);

			FCollisionResponseParams ResponseParams;
			ResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Ignore);

			FHitResult Hit;
			if (World->LineTraceSingleByChannel(Hit, Center, Origin, ECC_ProjectileTrace, Params, ResponseParams))
			{
				Origin = Hit.Location + (Center - Origin).GetSafeNormal() * 2.f;
			}
		}
	}

	// 부채꼴 꼭짓점(0,0) = 총구. 트레이스 시작점(GetComponentLocation)도 같이 따라감
	MeshComp->SetWorldLocation(Origin + FVector(0.f, 0.f, HeightOffset));
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

void ULumenAimLineComponent::StartTelegraph(float Duration)
{
	if (!MeshComp) return;

	bTelegraphMode = true;
	TelegraphElapsed = 0.f;
	TelegraphTotal = FMath::Max(Duration, 0.01f);

	// 첫 프레임부터 벽에 잘린 형태 + 투명 상태로 시작 (표시 전에 먼저 갱신 — 깜빡임 방지)
	FVector FlatDir = AimDirection;
	FlatDir.Z = 0.f;
	if (!FlatDir.IsNearlyZero())
	{
		FlatDir.Normalize();
		ApplyOrigin();
		MeshComp->SetWorldRotation(FlatDir.Rotation());
		UpdateRayLengths(FlatDir);
	}

	MeshComp->SetMeshSectionVisible(1, true);
	UpdateTelegraphVisual();
	MeshComp->SetVisibility(true);
}

void ULumenAimLineComponent::StopTelegraph()
{
	bTelegraphMode = false;
	TelegraphElapsed = 0.f;

	if (MeshComp)
	{
		MeshComp->SetVisibility(false);
		MeshComp->SetMeshSectionVisible(1, false);
	}
	SetIntensity(1.f);
}

void ULumenAimLineComponent::SetRangeMultiplier(float Multiplier)
{
	Multiplier = FMath::Max(Multiplier, 0.01f);
	if (FMath::IsNearlyEqual(Multiplier, RangeMultiplier)) return;
	RangeMultiplier = Multiplier;
	RebuildMesh();
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

	// 초록(LineTrace)=TraceRange, 빨강/파랑(Projectile)=Max Range(FalloffEndRange).
	// 두 값 모두 실제로 빛이 사라지는 지점이라 조준선 끝 = 소멸 지점
	OutLength = (CurrentWeapon->FireMode == EWeaponFireMode::LineTrace)
		? CurrentWeapon->TraceRange
		: CurrentWeapon->FalloffEndRange;
	OutLength *= RangeMultiplier; // 방별 사거리 배율 (각도는 그대로라 콘 폭도 같은 비율로 늘어남)

	const float HalfAngleRad = FMath::DegreesToRadians(CurrentWeapon->TelegraphWidthDegrees * 0.5f);
	OutHalfWidth = OutLength * FMath::Tan(HalfAngleRad);

	OutColor = CurrentWeapon->WeaponColor_TEMP;
}

float ULumenAimLineComponent::GetRayAngleDeg(int32 Index) const
{
	// Index 0 = 왼쪽 끝(-반각), RayCount-1 = 오른쪽 끝(+반각)
	const float T = (RayCount > 1) ? static_cast<float>(Index) / (RayCount - 1) : 0.5f;
	return FMath::Lerp(-CachedHalfAngleDeg, CachedHalfAngleDeg, T);
}

void ULumenAimLineComponent::UpdateRayLengths(const FVector& FlatDir)
{
	RayLengths.SetNum(RayCount);

	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!bClipByWorld || !World || !Owner || CachedLength <= 0.f)
	{
		for (float& L : RayLengths) L = CachedLength;
		return;
	}

	// 발사체와 같은 채널 — "빛이 실제로 막히거나 꺾이는 지점"에서 조준선도 끝나도록
	// (벽, 거울, 문, 상대의 방패에서 멈춤. 트리거 볼륨은 이 채널을 Ignore하도록 설정되어 있어 통과)
	FCollisionQueryParams Params(SCENE_QUERY_STAT(AimLineTrace), false, Owner);
	TArray<AActor*> AttachedActors;
	Owner->GetAttachedActors(AttachedActors); // 내 방패 등 붙어 있는 액터는 무시
	Params.AddIgnoredActors(AttachedActors);

	// 캐릭터는 통과 — 적이 사거리 안에 들어와 있는지 조준선 위에서 바로 읽히도록
	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Ignore);

	const FVector Start = MeshComp->GetComponentLocation();

	for (int32 i = 0; i < RayCount; ++i)
	{
		const FVector Dir = FlatDir.RotateAngleAxis(GetRayAngleDeg(i), FVector::UpVector);
		const FVector End = Start + Dir * CachedLength;

		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_ProjectileTrace, Params, ResponseParams))
		{
			RayLengths[i] = Hit.Distance;
		}
		else
		{
			RayLengths[i] = CachedLength;
		}
	}
}

void ULumenAimLineComponent::WriteFan(int32 SectionIndex, float MaxDistance, const FLinearColor& Color, bool bCreate)
{
	if (!MeshComp) return;

	const float Z = (SectionIndex == 1) ? 0.5f : 0.f; // 채움 콘을 배경보다 살짝 위에
	const float InvMax = (CachedLength > KINDA_SMALL_NUMBER) ? 1.f / CachedLength : 0.f;
	const int32 NumVerts = RayCount + 1;

	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> VertexColors;
	Vertices.Reserve(NumVerts);
	Normals.Init(FVector::UpVector, NumVerts);
	UVs.Reserve(NumVerts);
	VertexColors.Init(Color, NumVerts);

	// 0번 정점 = 꼭짓점(총구 위치), V=0.5(중심선)
	Vertices.Add(FVector(0.f, 0.f, Z));
	UVs.Add(FVector2D(0.f, 0.5f));

	for (int32 i = 0; i < RayCount; ++i)
	{
		float Dist = RayLengths.IsValidIndex(i) ? RayLengths[i] : CachedLength;
		if (MaxDistance >= 0.f)
		{
			Dist = FMath::Min(Dist, MaxDistance);
		}

		const float AngleRad = FMath::DegreesToRadians(GetRayAngleDeg(i));
		Vertices.Add(FVector(FMath::Cos(AngleRad) * Dist, FMath::Sin(AngleRad) * Dist, Z));

		const float V = (RayCount > 1) ? static_cast<float>(i) / (RayCount - 1) : 0.5f;
		UVs.Add(FVector2D(Dist * InvMax, V)); // U = 절대 거리 비율 → 벽에 잘려도 그라데이션이 늘어나지 않음
	}

	TArray<FProcMeshTangent> Tangents;

	if (bCreate)
	{
		TArray<int32> Triangles;
		Triangles.Reserve((RayCount - 1) * 3);
		for (int32 i = 0; i < RayCount - 1; ++i)
		{
			Triangles.Add(0);
			Triangles.Add(i + 1);
			Triangles.Add(i + 2);
		}
		MeshComp->CreateMeshSection_LinearColor(SectionIndex, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, false);
		if (DynamicMat)
		{
			MeshComp->SetMaterial(SectionIndex, DynamicMat);
		}
	}
	else
	{
		// 정점 수가 같으므로 섹션을 다시 만들지 않고 위치/UV/색만 갱신
		MeshComp->UpdateMeshSection_LinearColor(SectionIndex, Vertices, Normals, UVs, VertexColors, Tangents);
	}
}

void ULumenAimLineComponent::RebuildMesh()
{
	if (!MeshComp) return;

	float HalfWidth = 0.f;
	GetShapeFromWeapon(CachedLength, HalfWidth, CachedColor);
	CachedHalfAngleDeg = (CachedLength > KINDA_SMALL_NUMBER)
		? FMath::RadiansToDegrees(FMath::Atan(HalfWidth / CachedLength))
		: 0.f;

	// 광선 수: 빨강 5도 → 3개, 파랑 40도 → 15개, 초록 1도 → 2개 (간격 3도 기준)
	const float FullAngle = CachedHalfAngleDeg * 2.f;
	RayCount = FMath::Clamp(FMath::CeilToInt(FullAngle / RayAngleStepDegrees) + 1, 2, MaxRayCount);

	RayLengths.Init(CachedLength, RayCount);

	MeshComp->ClearAllMeshSections();

	// 0번: 콘 전체 — 플레이어 조준선은 이 섹션만 사용
	WriteFan(0, -1.f, CachedColor, true);

	// 1번: 텔레그래프 채움 콘 — 길이 0으로 만들어두고 텔레그래프 중에만 표시
	WriteFan(1, 0.f, CachedColor, true);
	MeshComp->SetMeshSectionVisible(1, bTelegraphMode);

	if (DynamicMat)
	{
		DynamicMat->SetVectorParameterValue(TEXT("TintColor"), CachedColor);
	}
	SetIntensity(1.f);

	if (bTelegraphMode)
	{
		UpdateTelegraphVisual();
	}
}

void ULumenAimLineComponent::UpdateTelegraphVisual()
{
	if (!MeshComp) return;

	const float D = TelegraphTotal;
	const float T = TelegraphElapsed;

	const float FadeInTime = D * FadeInRatio;
	const float Flash = FMath::Min(FlashTime, D * 0.25f);
	const float FlashStart = D - Flash;

	// ① 페이드인
	const float FadeAlpha = (FadeInTime > KINDA_SMALL_NUMBER) ? FMath::Clamp(T / FadeInTime, 0.f, 1.f) : 1.f;

	// ② 채움 — 페이드인이 끝난 뒤부터 번쩍 직전까지 0 → 1
	const float FillSpan = FMath::Max(FlashStart - FadeInTime, KINDA_SMALL_NUMBER);
	float FillProgress = FMath::Clamp((T - FadeInTime) / FillSpan, 0.f, 1.f);

	FLinearColor BgColor = CachedColor;
	BgColor.A = BackgroundAlpha * FadeAlpha;

	FLinearColor FillColor = CachedColor;
	FillColor.A = FMath::Lerp(FillAlphaStart, FillAlphaEnd, FillProgress) * FadeAlpha;

	float Intensity = 1.f;

	// ③ 번쩍 — 발사 직전 마지막 구간
	if (Flash > 0.f && T >= FlashStart)
	{
		FillProgress = 1.f;
		FillColor = FMath::Lerp(CachedColor, FLinearColor::White, FlashWhiten);
		FillColor.A = 1.f;
		BgColor.A = BackgroundAlpha;
		Intensity = FlashIntensity;
	}

	WriteFan(0, -1.f, BgColor, false);
	// 채움은 "적에게서의 거리"로 차오름 — 벽에 잘린 광선은 그 지점에서 멈춤
	WriteFan(1, FillProgress * CachedLength, FillColor, false);
	SetIntensity(Intensity);
}

void ULumenAimLineComponent::SetIntensity(float Value)
{
	// 머티리얼에 "Intensity" 파라미터가 없으면 아무 일도 안 일어남 (에러 없음)
	if (DynamicMat)
	{
		DynamicMat->SetScalarParameterValue(TEXT("Intensity"), Value);
	}
}