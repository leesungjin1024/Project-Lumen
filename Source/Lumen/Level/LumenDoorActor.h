// ALumenDoorActor — 위로 슬라이드하는 문. 전투/퍼즐 클리어 시 코드로 열리거나(자동), E키로 여닫음(bInteractable).
// 이동 로직은 C++에 두고 비주얼은 블루프린트 서브클래스에서 메시/이펙트만 갈아끼우는 구조.
// 연결: ACombatRoomVolume/APuzzleRoomVolume::ExitDoors, ILumenInteractable

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LumenInteractable.h"
#include "LumenDoorActor.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class USoundBase;

UCLASS()
class LUMEN_API ALumenDoorActor : public AActor, public ILumenInteractable
{
	GENERATED_BODY()

public:
	ALumenDoorActor();

	// ===== 여닫기 =====
	// 방 클리어/사망 리셋/상호작용에서 호출. 이미 그 상태면 무시
	UFUNCTION(BlueprintCallable, Category = "Door")
	void Open();

	UFUNCTION(BlueprintCallable, Category = "Door")
	void Close();

	UFUNCTION(BlueprintCallable, Category = "Door")
	bool IsOpen() const { return bIsOpen; }

	// E키로 직접 여닫는 문인지. 방 클리어로 열리는 문이면 false
	UPROPERTY(EditAnywhere, Category = "Door")
	bool bInteractable = false;

	// 상호작용 문일 때: 지금 열림/닫힘 상태를 그대로 뒤집음
	UFUNCTION(BlueprintCallable, Category = "Door")
	void ToggleInteract() { IsOpen() ? Close() : Open(); }

	// 문이 위로 슬라이드하는 거리
	UPROPERTY(EditAnywhere, Category = "Door")
	float OpenHeight = 300.f;

	// 여닫는 데 걸리는 시간(초)
	UPROPERTY(EditAnywhere, Category = "Door")
	float OpenDuration = 1.0f;

	// ===== 사운드 =====
	// 열릴 때. 비워두면 무음
	UPROPERTY(EditAnywhere, Category = "Door|Sound")
	TObjectPtr<USoundBase> OpenSound;

	// 닫힐 때. 비워두면 무음
	UPROPERTY(EditAnywhere, Category = "Door|Sound")
	TObjectPtr<USoundBase> CloseSound;

	// ===== ILumenInteractable =====
	virtual void Interact(ALumenCharacter* Interactor) override;
	virtual bool CanInteract() const override;
	virtual FText GetInteractPrompt() const override;
	virtual FVector GetPromptLocation() const override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// ===== 컴포넌트 =====
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	// 플레이어 캡슐이 들어오면 상호작용 후보로 등록되는 범위. bInteractable일 때만 사용
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionZone;

	UFUNCTION()
	void OnInteractionZoneBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractionZoneEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	// ===== 이동 상태 =====
	FVector ClosedLocation = FVector::ZeroVector;
	FVector TargetLocation = FVector::ZeroVector;
	bool bIsOpen = false;
	bool bIsMoving = false;
	float MoveElapsed = 0.f;
	FVector MoveStartLocation = FVector::ZeroVector;
	// 프롬프트 위치용 — 열려서 문이 이동해도 프롬프트는 닫힌 위치 위에 고정
	float ClosedTopZ = 0.f;
};