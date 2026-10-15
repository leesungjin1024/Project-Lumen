#include "LumenPlayerGaugesWidget.h"
#include "LumenCharacter.h"
#include "HeatComponent.h"
#include "Components/ProgressBar.h"

void ULumenPlayerGaugesWidget::Init(ALumenCharacter* InCharacter)
{
	Character = InCharacter;
	Heat = InCharacter ? InCharacter->FindComponentByClass<UHeatComponent>() : nullptr;

	if (ShieldBar) ShieldBar->SetRenderOpacity(0.f);
	if (HeatBar)   HeatBar->SetRenderOpacity(0.f);
	if (DashBar)   DashBar->SetRenderOpacity(0.f);

	if (InCharacter)
	{
		LastWeaponColor = InCharacter->GetActiveWeaponColor();   // 시작 시엔 "색 바뀜"으로 치지 않음
	}
}

void ULumenPlayerGaugesWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	ALumenCharacter* C = Character.Get();
	if (!C) return;

	BlinkTime += InDeltaTime;
	const bool bBlinkOn = FMath::Sin(BlinkTime * BlinkSpeed) > 0.f;

	// ===== 방패 (왼쪽) =====
	if (ShieldBar)
	{
		const float Ratio = C->GetShieldRatio();
		const bool bForcedCooldown = C->IsShieldOnForcedCooldown();

		ShieldBar->SetPercent(Ratio);

		FLinearColor Color = bForcedCooldown ? ShieldCooldownColor : NeutralColor;
		if (bForcedCooldown)
		{
			Color = ShieldCooldownColor;
			Color.A = bBlinkOn ? 1.f : 0.35f;
		}
		ShieldBar->SetFillColorAndOpacity(Color);

		const bool bActive = C->IsShieldActive() || bForcedCooldown || Ratio < 0.999f;
		UpdateFade(ShieldFade, ShieldBar, bActive, InDeltaTime);
	}

	// ===== 발열 (오른쪽) =====
	if (HeatBar)
	{
		const FLinearColor WeaponColor = C->GetActiveWeaponColor();
		if (!WeaponColor.Equals(LastWeaponColor))
		{
			LastWeaponColor = WeaponColor;
			ColorChangeShowTimer = HideDelay;   // 색 바꾸면 잠깐 보여줌
		}
		ColorChangeShowTimer = FMath::Max(0.f, ColorChangeShowTimer - InDeltaTime);

		float Ratio = 0.f;
		bool bOverheated = false;
		if (UHeatComponent* H = Heat.Get())
		{
			Ratio = H->GetHeatRatio();
			bOverheated = H->IsOverheated();
		}

		HeatBar->SetPercent(Ratio);
		// 과열 중엔 고정 색, 끝나면 무기 색으로 복귀 (깜빡임 없음)
		HeatBar->SetFillColorAndOpacity(bOverheated ? OverheatFlashColor : WeaponColor);

		const bool bActive = bOverheated || Ratio > 0.001f || ColorChangeShowTimer > 0.f;
		UpdateFade(HeatFade, HeatBar, bActive, InDeltaTime);
	}

	// ===== 구르기 (아래) =====
	if (DashBar)
	{
		const float Ratio = C->GetDashReadyRatio();   // 0 = 막 씀, 1 = 사용 가능
		DashBar->SetPercent(Ratio);
		DashBar->SetFillColorAndOpacity(NeutralColor);

		UpdateFade(DashFade, DashBar, Ratio < 0.999f, InDeltaTime);
	}
}

void ULumenPlayerGaugesWidget::UpdateFade(FLumenGaugeFade& Fade, UWidget* Target, bool bActive, float DeltaTime)
{
	if (!Target) return;

	if (bActive)
	{
		Fade.IdleTime = 0.f;
		Fade.Opacity = FMath::FInterpConstantTo(Fade.Opacity, 1.f, DeltaTime, FadeInSpeed);
	}
	else
	{
		Fade.IdleTime += DeltaTime;
		if (Fade.IdleTime >= HideDelay)
		{
			Fade.Opacity = FMath::FInterpConstantTo(Fade.Opacity, 0.f, DeltaTime, FadeOutSpeed);
		}
	}

	Target->SetRenderOpacity(Fade.Opacity * MaxOpacity);
}
