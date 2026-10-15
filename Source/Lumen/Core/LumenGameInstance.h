// ULumenGameInstance — 진행 세이브/로드(체크포인트 1슬롯)와 볼륨 설정의 사운드 믹스 적용. 레벨 전환 후에도 로드 데이터를 보관.
// 연결: ULumenSaveGame, ALumenPlayerState, ACheckpointActor, ULumenGameUserSettings
#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "LumenGameInstance.generated.h"

class ULumenSaveGame;
class ALumenPlayerState;
class USoundMix;
class USoundClass;


UCLASS()
class LUMEN_API ULumenGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// ===== 세이브 =====
	static const FString SaveSlotName;
	static const int32 SaveUserIndex = 0;

	UFUNCTION(BlueprintCallable, Category = "Save")
	bool HasSaveGame() const;

	// 새 체크포인트에 도달할 때마다 호출 — 디스크에 즉시 기록
	UFUNCTION(BlueprintCallable, Category = "Save")
	void SaveCurrentProgress(ALumenPlayerState* PlayerState);

	// 타이틀에서 Continue 클릭 시 호출 — 로드해서 멤버에 보관 (레벨 전환 이후까지 살아있음)
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool LoadSavedProgress();

	// 레벨 로드 후 PlayerState가 이 값을 가져가서 적용. 한 번 가져가면 비워짐
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool ConsumePendingLoad(FTransform& OutTransform, FName& OutCheckpointID, int32& OutCheckpointOrder);

	// 타이틀로 나갈 때 호출 — 남은 로드 대기 데이터 폐기
	UFUNCTION(BlueprintCallable)
	void ClearPendingLoad() { bHasPendingLoad = false; }

	// ===== 오디오 =====
	// ULumenGameUserSettings의 볼륨을 사운드 믹스에 적용 (게임 시작 시 + 설정 변경 시)
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void ApplyAudioSettings();

	virtual void OnStart() override;
private:
	UPROPERTY()
	TObjectPtr<ULumenSaveGame> PendingLoadedSave = nullptr;

	bool bHasPendingLoad = false;

	// 볼륨 오버라이드를 걸 사운드 믹스. 비우면 볼륨 설정이 적용되지 않음
	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TObjectPtr<USoundMix> MasterSoundMix;

	// 배경음/효과음 외 기타 소리(UI 등)용 마스터 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TObjectPtr<USoundClass> MasterSoundClass;

	// SC_Music / SC_SFX — 둘 다 Parent Class 없이 독립 클래스로 둘 것 (마스터 볼륨은 코드에서 곱함)
	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TObjectPtr<USoundClass> MusicSoundClass;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TObjectPtr<USoundClass> SFXSoundClass;
};