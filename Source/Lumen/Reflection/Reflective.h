// IReflective — 빛을 반사하는 액터의 인터페이스. 반사 노멀만 제공하고 반사 계산은 호출 측에서 함.
// 연결: AMirrorActor, AShieldActor / AProjectile::HandleHit, ULumenCombatLibrary::FireReflectiveLineTrace
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Reflective.generated.h"


UINTERFACE(MinimalAPI, Blueprintable)
class UReflective : public UInterface
{
	GENERATED_BODY()
};

class LUMEN_API IReflective
{
	GENERATED_BODY()

public:
	// 현재 각도의 반사 노멀 반환. 거울/방패 모두 액터 정면(forward)을 Z=0으로 평탄화해 수평 반사만 하도록 구현
	virtual FVector GetReflectionNormal(const FHitResult& Hit) const = 0;
};