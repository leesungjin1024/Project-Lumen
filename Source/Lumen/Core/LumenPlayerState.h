// ALumenPlayerState — 마지막 체크포인트 데이터 보관. 로드된 세이브가 있으면 BeginPlay에서 가져와 적용.
// 연결: ACheckpointActor(기록), ULumenGameInstance(세이브/로드), ALumenCharacter(리스폰 위치)
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "LumenPlayerState.generated.h"

UCLASS()
class LUMEN_API ALumenPlayerState : public APlayerState
{
	GENERATED_BODY()
public:
	// ===== 체크포인트 =====
	// 리스폰 위치/방향 (위치·회전만 사용)
	UPROPERTY(BlueprintReadOnly)
	FTransform LastCheckpointTransform;
	UPROPERTY(BlueprintReadOnly)
	FName LastCheckpointID;
	// 지금까지 도달한 가장 진행된 체크포인트 순서. -1 = 아직 없음
	UPROPERTY(BlueprintReadOnly)
	int32 LastCheckpointOrder = -1;
	// 세이브 구조가 나중에 바뀌어도 마이그레이션 분기가 가능하도록 세이브에 함께 기록
	UPROPERTY(BlueprintReadOnly)
	int32 SaveVersion = 1;

protected:
	// 로드된 세이브가 있으면 여기서 필드에 적용
	virtual void BeginPlay() override;
};