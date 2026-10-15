#include "LumenHealthChunksWidget.h"
#include "HealthComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

void ULumenHealthChunksWidget::Init(UHealthComponent* InHealth, bool bInOnlyWhenTargeted)
{
    HealthComp = InHealth;
    bOnlyWhenTargeted = bInOnlyWhenTargeted;
    BuildHearts();
}

void ULumenHealthChunksWidget::BuildHearts()
{
    if (!HeartBox || !HealthComp.IsValid()) return;

    HeartBox->ClearChildren();
    Hearts.Reset();

    for (int32 i = 0; i < HealthComp->NumHealthChunks; ++i)
    {
        UImage* Img = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
        Img->SetDesiredSizeOverride(HeartSize);
        if (UHorizontalBoxSlot* HSlot = HeartBox->AddChildToHorizontalBox(Img))
        {
            HSlot->SetPadding(FMargin(HeartSpacing * 0.5f, 0.f));
        }
        Hearts.Add(Img);
    }
    LastFilled = -1;   // 다음 틱에 강제 갱신
}

void ULumenHealthChunksWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (!HealthComp.IsValid()) return;

    // 표시 여부 (적: 조준 중일 때만)
    const bool bShow = !bOnlyWhenTargeted || HealthComp->IsTargeted();
    SetRenderOpacity(bShow ? 1.f : 0.f);

    const int32 Filled = HealthComp->GetFilledChunkCount();
    if (Filled != LastFilled)
    {
        Refresh(Filled);
        LastFilled = Filled;
    }
}

void ULumenHealthChunksWidget::Refresh(int32 Filled)
{
    for (int32 i = 0; i < Hearts.Num(); ++i)
    {
        const bool bFull = i < Filled;
        Hearts[i]->SetBrushFromTexture(bFull ? HeartFullTexture : HeartEmptyTexture);
        Hearts[i]->SetColorAndOpacity(bFull ? FullColor : EmptyColor);
    }
}