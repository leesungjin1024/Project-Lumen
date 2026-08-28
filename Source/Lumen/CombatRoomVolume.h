// CombatRoomVolume.h
// 전투룸: 적 스폰 관리 + 전멸 판정 + 출구 문 개방
// 의존: ALumenEnemyCharacter, UHealthComponent (OnDeath 델리게이트, 기존 4주차 구현 재사용)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoomVolume.h"
#include "CombatRoomVolume.generated.h"

class ALumenEnemyCharacter;
class UBoxComponent;
class ALumenDoorActor;

UCLASS()
class LUMEN_API ACombatRoomVolume : public ARoomVolume
{
	GENERATED_BODY()

public:
	ACombatRoomVolume();

	// 스폰할 적 클래스 목록 (에디터에서 지정)
	UPROPERTY(EditAnywhere, Category = "CombatRoom")
	TArray<TSubclassOf<ALumenEnemyCharacter>> EnemyClassesToSpawn;

	// 스폰 위치 (레벨에서 직접 배치한 트랜스폼)
	UPROPERTY(EditAnywhere, Category = "CombatRoom")
	TArray<FTransform> SpawnPoints;

	// 클리어 시 열릴 문 액터
	UPROPERTY(EditAnywhere, Category = "CombatRoom")
	TArray<TObjectPtr<ALumenDoorActor>> ExitDoors;

	// 진입 감지용 트리거 볼륨
	UPROPERTY(VisibleAnywhere, Category = "CombatRoom")
	UBoxComponent* EntryTrigger;

	// ARoomVolume 오버라이드 — 플레이어가 방 안에서 죽었을 때 호출됨
	virtual void ResetRoom() override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TArray<ALumenEnemyCharacter*> AliveEnemies;

	// OnDeath 델리게이트가 파라미터 없는(void) 시그니처라 "누가 죽었는지"를 직접 못 받으므로
	// 살아있는 수를 카운터로 관리 (AliveEnemies 배열은 스폰 목록 보관용으로만 사용)
	int32 AliveEnemyCount = 0;

	bool bRoomStarted = false;

	UFUNCTION()
	void OnEntryOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	// 방 밖으로 나갔을 때 CurrentRoom을 지워주기 위한 EndOverlap (신규)
	UFUNCTION()
	void OnEntryEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void StartRoom();
	void SpawnEnemies();

	// OnDeath 델리게이트 시그니처(파라미터 없음)와 맞춰야 AddDynamic이 컴파일됨
	UFUNCTION()
	void HandleEnemyDeath();

	void OpenExitDoor();
};