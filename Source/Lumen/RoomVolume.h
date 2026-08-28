// RoomVolume.h
// 전투룸/퍼즐룸의 공통 부모 — "지금 플레이어가 어느 방에 있는지"를 방 종류와 무관하게
// 하나의 포인터로 추적하고, 죽었을 때 ResetRoom()만 부르면 되게 하기 위한 최소 인터페이스.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoomVolume.generated.h"

UCLASS(Abstract)
class LUMEN_API ARoomVolume : public AActor
{
	GENERATED_BODY()

public:
	// 플레이어가 이 방 안에서 죽었을 때 호출 — 각 방 종류에 맞게 오버라이드
	virtual void ResetRoom() {}
};
