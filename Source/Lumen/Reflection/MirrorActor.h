// MirrorActor.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Reflective.h"
#include "MirrorActor.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class USphereComponent;

UCLASS()
class LUMEN_API AMirrorActor : public AActor, public IReflective
{
	GENERATED_BODY()

public:
	AMirrorActor();

	virtual FVector GetReflectionNormal(const FHitResult& Hit) const override;

	// 5-3: E키 상호작용으로 각도 조절 - 캐릭터의 Interact()에서 호출
	UFUNCTION(BlueprintCallable, Category = "Reflection")
	void RotateStep();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnInteractionZoneBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractionZoneEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> CollisionComp;

	// 상호작용 가능 범위 - 반사 판정용 CollisionComp와는 별개
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionZone;

	// 5-3: 정적/회전 가능 거울 둘 다 이 클래스로 표현, 이 플래그로 구분
	UPROPERTY(EditAnywhere, Category = "Reflection")
	bool bIsRotatable = false;

	// 한 번 상호작용할 때 회전하는 각도 (5-6 퍼즐에서 각도 조절 단위)
	UPROPERTY(EditAnywhere, Category = "Reflection", meta = (EditCondition = "bIsRotatable"))
	float RotationStepDegrees = 45.f;
};
