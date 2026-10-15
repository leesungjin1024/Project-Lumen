#include "LumenGameUserSettings.h"

ULumenGameUserSettings* ULumenGameUserSettings::GetLumenSettings()
{
    return Cast<ULumenGameUserSettings>(UGameUserSettings::GetGameUserSettings());
}

void ULumenGameUserSettings::SetToDefaults()
{
    Super::SetToDefaults();
    MasterVolume = 1.f;
    MusicVolume = 1.f;
    SFXVolume = 1.f;
}