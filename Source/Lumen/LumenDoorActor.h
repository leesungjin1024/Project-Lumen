// LumenDoorActor.h
// 전투룸/퍼즐룸 클리어 시 열리는 문. 최소 구현 — 시각적 폴리싱은 나중에 블루프린트 서브클래스에서
// 메시/이펙트만 갈아끼우면 되도록, 이동 로직은 C++ 코어에 두고 비주얼은 컴포넌트에 맡김.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LumenDoorActor.generated.h"

class UStaticMeshComponent;
class USphereComponent;

UCLASS()
class LUMEN_API ALumenDoorActor : public AActor
{
	GENERATED_BODY()

public:
	ALumenDoorActor();

	// CombatRoomVolume / PuzzleSensorActor / 상호작용에서 호출하는 진입점
	UFUNCTION(BlueprintCallable, Category = "Door")
	void Open();

	UFUNCTION(BlueprintCallable, Category = "Door")
	void Close();

	UFUNCTION(BlueprintCallable, Category = "Door")
	bool IsOpen() const { return bIsOpen; }

	// E키로 직접 여닫는 상호작용용 문인지 (자동 클리어 문이면 false로 두고 안 씀)
	UPROPERTY(EditAnywhere, Category = "Door")
	bool bInteractable = false;

	// 상호작용 문일 때: 지금 열림/닫힘 상태를 그대로 뒤집음
	UFUNCTION(BlueprintCallable, Category = "Door")
	void ToggleInteract() { IsOpen() ? Close() : Open(); }

	// 문이 위로 슬라이드하는 거리 (기본: 문 높이만큼 위로)
	UPROPERTY(EditAnywhere, Category = "Door")
	float OpenHeight = 300.f;

	UPROPERTY(EditAnywhere, Category = "Door")
	float OpenDuration = 1.0f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	// bInteractable일 때만 사용 — AMirrorActor의 InteractionZone과 동일한 패턴
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionZone;

	UFUNCTION()
	void OnInteractionZoneBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractionZoneEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	FVector ClosedLocation = FVector::ZeroVector;
	FVector TargetLocation = FVector::ZeroVector;
	bool bIsOpen = false;
	bool bIsMoving = false;
	float MoveElapsed = 0.f;
	FVector MoveStartLocation = FVector::ZeroVector;
};