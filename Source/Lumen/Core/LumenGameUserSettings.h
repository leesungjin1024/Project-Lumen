// ULumenGameUserSettings — 볼륨 설정(마스터/배경음/효과음, 0~1)을 GameUserSettings.ini에 저장. 진행 세이브와는 분리.
// 연결: ULumenGameInstance::ApplyAudioSettings
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "LumenGameUserSettings.generated.h"

UCLASS(BlueprintType,config = GameUserSettings, configdonotcheckdefaults)
class LUMEN_API ULumenGameUserSettings : public UGameUserSettings
{
    GENERATED_BODY()

public:
    // 엔진 GameUserSettings를 이 클래스로 캐스팅해 반환 (설정 클래스가 이 클래스로 지정돼 있어야 함)
    UFUNCTION(BlueprintCallable, Category = "Lumen|Settings")
    static ULumenGameUserSettings* GetLumenSettings();

    // ===== 볼륨 =====

    UFUNCTION(BlueprintCallable, Category = "Lumen|Settings")
    void SetMasterVolume(float InVolume) { MasterVolume = FMath::Clamp(InVolume, 0.f, 1.f); }

    UFUNCTION(BlueprintPure, Category = "Lumen|Settings")
    float GetMasterVolume() const { return MasterVolume; }

    // 배경음 / 효과음 볼륨 (최종 볼륨 = 마스터 × 각 값)
    UFUNCTION(BlueprintCallable, Category = "Lumen|Settings")
    void SetMusicVolume(float InVolume) { MusicVolume = FMath::Clamp(InVolume, 0.f, 1.f); }

    UFUNCTION(BlueprintPure, Category = "Lumen|Settings")
    float GetMusicVolume() const { return MusicVolume; }

    UFUNCTION(BlueprintCallable, Category = "Lumen|Settings")
    void SetSFXVolume(float InVolume) { SFXVolume = FMath::Clamp(InVolume, 0.f, 1.f); }

    UFUNCTION(BlueprintPure, Category = "Lumen|Settings")
    float GetSFXVolume() const { return SFXVolume; }

    // 볼륨도 모두 1로 되돌림
    virtual void SetToDefaults() override;
private:
    UPROPERTY(Config)
    float MasterVolume = 1.f;

    UPROPERTY(Config)
    float MusicVolume = 1.f;

    UPROPERTY(Config)
    float SFXVolume = 1.f;
};