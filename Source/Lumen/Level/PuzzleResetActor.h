// APuzzleResetActor — 퍼즐룸 입구의 리셋 오브젝트. E키로 그 방의 회전 거울 전체를 배치 시점 각도로 되돌림 (방이 클리어되면 비활성).
// 연결: ILumenInteractable, APuzzleRoomVolume::ResetMirrors

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LumenInteractable.h"
#include "PuzzleResetActor.generated.h"   // 항상 마지막 include

class UStaticMeshComponent;
class USphereComponent;
class APuzzleRoomVolume;
class USoundBase;

UCLASS()
class LUMEN_API APuzzleResetActor : public AActor, public ILumenInteractable
{
	GENERATED_BODY()

public:
	APuzzleResetActor();

	// ILumenInteractable
	virtual void Interact(ALumenCharacter* Interactor) override;
	virtual bool CanInteract() const override;
	virtual FText GetInteractPrompt() const override;
	virtual FVector GetPromptLocation() const override;

protected:
	virtual void BeginPlay() override;

	// ===== 컴포넌트 =====
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> MeshComp;

	// 플레이어 캡슐이 들어오면 상호작용 후보로 등록되는 범위
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> InteractionZone;

	// ===== 퍼즐 =====
	// 거울을 초기화할 퍼즐룸 (반드시 지정)
	UPROPERTY(EditInstanceOnly, Category = "Puzzle")
	TObjectPtr<APuzzleRoomVolume> OwningRoom;

	// 연타 방지용 재사용 대기 시간(초)
	UPROPERTY(EditAnywhere, Category = "Puzzle")
	float ResetCooldown = 0.5f;

	// ===== 사운드 =====
	// 거울 초기화 시. 비워두면 무음
	UPROPERTY(EditAnywhere, Category = "Puzzle|Sound")
	TObjectPtr<USoundBase> ResetSound;

private:
	UFUNCTION()
	void OnZoneBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnZoneEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	double LastResetTime = -1000.0;
};