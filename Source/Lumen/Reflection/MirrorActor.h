// AMirrorActor — 맵 거울. 정적/회전 가능(bIsRotatable)을 한 클래스로 표현. 회전 거울은 E 짧게=정방향, 길게=역방향으로 Yaw만 회전.
// 연결: IReflective(AProjectile/ULumenCombatLibrary 반사), ILumenInteractable, APuzzleRoomVolume::ResetMirrors
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Reflective.h"
#include "LumenInteractable.h"
#include "MirrorActor.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class USphereComponent;

UCLASS()
class LUMEN_API AMirrorActor : public AActor, public IReflective, public ILumenInteractable
{
	GENERATED_BODY()

public:
	AMirrorActor();

	virtual void Tick(float DeltaTime) override;

	virtual FVector GetReflectionNormal(const FHitResult& Hit) const override;

	// Direction: +1 정방향(E 짧게 누름), -1 역방향(E 길게 누름)
	UFUNCTION(BlueprintCallable, Category = "Reflection")
	void RotateStep(int32 Direction = 1);

	// 배치 시 각도로 되돌림 — 퍼즐 리셋(APuzzleRoomVolume::ResetMirrors)에서 호출
	void ResetToInitialRotation();

	bool IsRotating() const { return bIsRotating; }

	// 이 거울에서 반사된 빛이 새로 받는 사거리 배율 (무기 기본 사거리 × 이 값). 발사체/초록 공통
	float GetReflectedRangeMultiplier() const { return ReflectedRangeMultiplier; }

	// ILumenInteractable — 회전 가능 거울만 상호작용 대상 (정적 거울은 오버랩 바인딩 자체를 안 함)
	virtual void Interact(ALumenCharacter* Interactor) override;
	virtual void InteractReleased(ALumenCharacter* Interactor) override;
	virtual void InteractCanceled() override;
	virtual bool CanInteract() const override;
	virtual FText GetInteractPrompt() const override;
	virtual FVector GetPromptLocation() const override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnInteractionZoneBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractionZoneEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// ===== 컴포넌트 =====
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComp;

	// 반사 판정 박스 (ECC_Reflective, ProjectileTrace Block, Visibility Ignore)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> CollisionComp;

	// 상호작용 감지용 구체 - 반사 판정용 CollisionComp와는 별개
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionZone;

	// ===== 회전 =====
	// 켜면 E로 회전 가능한 거울, 끄면 정적 거울 (상호작용 대상에서도 빠짐)
	UPROPERTY(EditAnywhere, Category = "Reflection")
	bool bIsRotatable = false;

	// 한 번 상호작용할 때 회전하는 각도
	UPROPERTY(EditAnywhere, Category = "Reflection", meta = (EditCondition = "bIsRotatable"))
	float RotationStepDegrees = 45.f;

	// 초당 회전 속도 — 240이면 45도 회전에 약 0.19초
	UPROPERTY(EditAnywhere, Category = "Reflection", meta = (EditCondition = "bIsRotatable", ClampMin = "1.0"))
	float RotationSpeedDegrees = 240.f;

	// 이 시간(초) 이상 E를 누르고 있으면 역방향 회전
	UPROPERTY(EditAnywhere, Category = "Reflection", meta = (EditCondition = "bIsRotatable", ClampMin = "0.1"))
	float MirrorReverseHoldTime = 0.35f;

	// ===== 반사 사거리 =====
	// 이 거울에 반사되면 남은 사거리가 "무기 기본 사거리 × 이 값"으로 새로 채워짐.
	// 1 = 기본 사거리로 리셋(이전 동작), 2 = 두 배. 거울마다 다르게 줘서 퍼즐 거리 조절 가능.
	// 반사될 때마다 무기 기본 사거리 기준으로 다시 계산 (거울을 여러 번 거쳐도 누적 곱셈은 안 됨)
	UPROPERTY(EditAnywhere, Category = "Reflection", meta = (ClampMin = "0.1"))
	float ReflectedRangeMultiplier = 2.f;

private:
	void StartRotatingTo(const FRotator& NewTarget);
	void FinishRotation();

	FRotator InitialRotation; // BeginPlay 시점 각도 (리셋 기준)
	FRotator TargetRotation;
	bool bIsRotating = false;

	// E 홀드 판정
	bool bPressPending = false;
	double PressStartTime = 0.0;
};