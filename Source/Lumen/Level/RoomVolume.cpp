#include "RoomVolume.h"
#include "LumenCharacter.h"

void ARoomVolume::ApplyRoomView(ALumenCharacter* Player, const FBox& RoomBounds)
{
    if (!Player) return;
    Player->EnterRoomView(this, bOverrideCameraPitch, CameraPitch,
        CameraArmLength, bMaskOutsideRoom, RoomBounds);
}

void ARoomVolume::ClearRoomView(ALumenCharacter* Player)
{
    if (Player) Player->ExitRoomView(this);
}