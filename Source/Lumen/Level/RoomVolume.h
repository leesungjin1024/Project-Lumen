// ARoomVolume — 전투룸/퍼즐룸의 공통 부모. 플레이어의 현재 방을 방 종류와 무관하게 하나의 포인터로 추적하고,
// 사망 시 ResetRoom()만 부르면 되게 한다. 방 진입/이탈 시 카메라 연출(피치/거리/방 밖 마스킹)도 적용.
// 연결: ALumenCharacter::CurrentRoom / EnterRoomView / ExitRoomView, ACombatRoomVolume, APuzzleRoomVolume

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoomVolume.generated.h"

class ALumenCharacter;

UCLASS(Abstract)
class LUMEN_API ARoomVolume : public AActor
{
	GENERATED_BODY()

public:
	// 플레이어가 이 방 안에서 죽었을 때 호출 — 각 방 종류에 맞게 오버라이드
	virtual void ResetRoom() {}

	// 이 방 안에서 플레이어 사거리(발사체 Max Range / 초록 TraceRange / 조준선 길이)에 곱하는 배율
	float GetPlayerRangeMultiplier() const { return PlayerRangeMultiplier; }

protected:
    // ===== 카메라 =====
    // 켜면 이 방 안에서 카메라 피치를 CameraPitch로 바꾼다. 끄면 기본 피치 유지
    UPROPERTY(EditAnywhere, Category = "Room|Camera")
    bool bOverrideCameraPitch = false;

    // 이 방 안에서 쓸 카메라 피치(도)
    UPROPERTY(EditAnywhere, Category = "Room|Camera", meta = (EditCondition = "bOverrideCameraPitch"))
    float CameraPitch = -89.f;

    // 이 방 안에서 쓸 카메라 거리. 0 이하면 기본 거리 유지
    UPROPERTY(EditAnywhere, Category = "Room|Camera")
    float CameraArmLength = -1.f;

    // 켜면 방 범위 밖을 가린다(마스킹)
    UPROPERTY(EditAnywhere, Category = "Room|Camera")
    bool bMaskOutsideRoom = true;

    // ===== 사거리 =====
    // 이 방 안에서 플레이어의 사거리 배율 (1 = 무기 DA 그대로). 퍼즐룸은 기본 3 — 거울 배치 거리 확보용.
    // 적/광원(ALightEmitterActor)에는 적용되지 않음
    UPROPERTY(EditAnywhere, Category = "Room|Range", meta = (ClampMin = "0.1"))
    float PlayerRangeMultiplier = 1.f;

    // 방 진입 시 카메라 설정 적용 / 이탈 시 해제. 해제는 지금 카메라를 차지한 방이 자신일 때만 적용됨
    void ApplyRoomView(ALumenCharacter* Player, const FBox& RoomBounds);
    void ClearRoomView(ALumenCharacter* Player);
};
