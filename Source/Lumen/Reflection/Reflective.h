// Reflective.h
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
	// 정적 거울은 고정된 표면 노멀만 반환, 회전 가능 거울/방패는 현재 각도의 노멀 반환
	virtual FVector GetReflectionNormal(const FHitResult& Hit) const = 0;
};