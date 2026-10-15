// ACombatRoomVolume — 전투룸. 플레이어 첫 진입 시 적(고정 포탑) 스폰, 전멸하면 출구 문 개방, 방 안에서 사망하면 리셋.
// 연결: ALumenEnemyCharacter, UHealthComponent::OnDeath, ALumenDoorActor(ExitDoors), ULumenMusicSubsystem

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoomVolume.h"
#include "CombatRoomVolume.generated.h"

class ALumenEnemyCharacter;
class UBoxComponent;
class ALumenDoorActor;
class USoundBase;

UCLASS()
class LUMEN_API ACombatRoomVolume : public ARoomVolume
{
	GENERATED_BODY()

public:
	ACombatRoomVolume();

	// ===== 스폰 =====
	// 스폰할 적 클래스 목록. SpawnPoints와 같은 인덱스끼리 짝지어 스폰되며, 둘 중 짧은 쪽 개수만큼만 스폰
	UPROPERTY(EditAnywhere, Category = "CombatRoom")
	TArray<TSubclassOf<ALumenEnemyCharacter>> EnemyClassesToSpawn;

	// 스폰 위치/회전 (EnemyClassesToSpawn과 같은 인덱스)
	UPROPERTY(EditAnywhere, Category = "CombatRoom")
	TArray<FTransform> SpawnPoints;

	// ===== 문 =====
	// 전멸 시 열리고, 사망 리셋 시 다시 닫히는 문
	UPROPERTY(EditAnywhere, Category = "CombatRoom")
	TArray<TObjectPtr<ALumenDoorActor>> ExitDoors;

	// ===== 사운드 =====
	// 적이 스폰되면 재생, 전멸/사망 리셋 시 레벨 기본 곡으로 복귀. 비우면 곡 안 바뀜
	UPROPERTY(EditAnywhere, Category = "CombatRoom|Audio")
	TObjectPtr<USoundBase> CombatMusic;

	// ===== 진입 감지 =====
	// 방 전체를 덮는 진입 감지 트리거. 이 범위가 방 카메라/마스킹 범위로도 쓰임
	UPROPERTY(VisibleAnywhere, Category = "CombatRoom")
	UBoxComponent* EntryTrigger;

	// 플레이어가 방 안에서 죽었을 때 호출. 이미 클리어된 방이면 아무것도 안 함
	virtual void ResetRoom() override;

protected:
	virtual void BeginPlay() override;

	// ===== 상태 =====
	// 스폰한 적 목록 (리셋 시 Destroy용)
	UPROPERTY()
	TArray<ALumenEnemyCharacter*> AliveEnemies;

	// OnDeath 델리게이트가 파라미터 없는(void) 시그니처라 "누가 죽었는지"를 직접 못 받으므로
	// 살아있는 수를 카운터로 관리 (AliveEnemies 배열은 스폰 목록 보관용으로만 사용)
	int32 AliveEnemyCount = 0;

	// 이번 진입에서 이미 적을 스폰했는지. 리셋 시 false로 되돌려 재입장 때 다시 스폰
	bool bRoomStarted = false;

	// ===== 내부 처리 =====
	UFUNCTION()
	void OnEntryOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	// 방 밖으로 나갔을 때 카메라 해제 + CurrentRoom을 지워주기 위한 EndOverlap
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