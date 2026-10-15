// ULumenPlayerGaugesWidget — 캐릭터 주변 게이지(왼쪽 방패 / 오른쪽 발열 / 아래 대쉬). WBP_PlayerGauges의 부모.
// Screen Space 위젯 컴포넌트로 표시되며, 쓸 때만 나타나고 가만히 있으면 사라짐.
// 연결: ALumenCharacter(방패/대쉬/무기 색), UHeatComponent

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LumenPlayerGaugesWidget.generated.h"

class UProgressBar;
class UWidget;
class ALumenCharacter;
class UHeatComponent;

// 게이지 하나의 페이드 상태 (UPROPERTY 아님 — 순수 런타임 값)
struct FLumenGaugeFade
{
	float Opacity = 0.f;
	float IdleTime = 100.f;   // 시작 시 숨김 상태
};

UCLASS()
class LUMEN_API ULumenPlayerGaugesWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 표시할 캐릭터 연결. 게이지는 전부 숨김 상태로 시작
	void Init(ALumenCharacter* InCharacter);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ===== WBP에서 이 이름 그대로 만들어야 함 =====
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UProgressBar> ShieldBar;  // 왼쪽, Bottom to Top
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UProgressBar> HeatBar;    // 오른쪽, Bottom to Top
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UProgressBar> DashBar;    // 아래, Left to Right

	// ===== 표시 규칙 =====
	// 사용이 끝난 뒤(가득 참/식음/쿨 끝) 이 시간만큼 더 보이다가 사라짐
	UPROPERTY(EditAnywhere, Category = "Gauges") float HideDelay = 0.8f;
	UPROPERTY(EditAnywhere, Category = "Gauges") float FadeInSpeed = 12.f;   // 초당 불투명도 변화량
	UPROPERTY(EditAnywhere, Category = "Gauges") float FadeOutSpeed = 4.f;
	// 조준선과 겹쳐도 덜 거슬리게 최대 불투명도 제한
	UPROPERTY(EditAnywhere, Category = "Gauges") float MaxOpacity = 0.85f;
	// 방패 강제 쿨타임 중 깜빡임 속도
	UPROPERTY(EditAnywhere, Category = "Gauges") float BlinkSpeed = 10.f;

	// ===== 색 =====
	// 방패/대쉬 게이지 색. 빨강/초록/파랑은 무기 전용이라 발열 게이지만 현재 무기 색을 씀
	UPROPERTY(EditAnywhere, Category = "Gauges|Color")
	FLinearColor NeutralColor = FLinearColor::FromSRGBColor(FColor(0xE8, 0xE6, 0xE1));

	// 방패 강제 쿨타임 중 색 (깜빡임)
	UPROPERTY(EditAnywhere, Category = "Gauges|Color")
	FLinearColor ShieldCooldownColor = FLinearColor(0.3f, 0.3f, 0.3f, 1.f);

	// 과열 중 발열 게이지 색 (깜빡이지 않고 고정)
	UPROPERTY(EditAnywhere, Category = "Gauges|Color")
	FLinearColor OverheatFlashColor = FLinearColor::White;

private:
	// bActive면 페이드인, 아니면 HideDelay 뒤 페이드아웃
	void UpdateFade(FLumenGaugeFade& Fade, UWidget* Target, bool bActive, float DeltaTime);

	TWeakObjectPtr<ALumenCharacter> Character;
	TWeakObjectPtr<UHeatComponent> Heat;

	FLumenGaugeFade ShieldFade;
	FLumenGaugeFade HeatFade;
	FLumenGaugeFade DashFade;

	float BlinkTime = 0.f;

	// 무기 색을 바꾸면 발열 게이지를 잠깐 보여줘서 "지금 무슨 색인지" 알려줌
	FLinearColor LastWeaponColor = FLinearColor::Transparent;
	float ColorChangeShowTimer = 0.f;
};
