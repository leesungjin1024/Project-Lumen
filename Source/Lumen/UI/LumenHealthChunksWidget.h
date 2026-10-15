// ULumenHealthChunksWidget — 머리 위 하트 칸 HP 표시(기본 3칸). 플레이어는 항상, 적은 조준 중일 때만 표시.
// 연결: UHealthComponent(NumHealthChunks/GetFilledChunkCount/IsTargeted), ALumenCharacter, ALumenEnemyCharacter
#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LumenHealthChunksWidget.generated.h"

class UHorizontalBox;
class UImage;
class UTexture2D;
class UHealthComponent;

UCLASS()
class LUMEN_API ULumenHealthChunksWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 표시할 HealthComponent 연결 + 하트 생성. bOnlyWhenTargeted: 적=true(조준 중일 때만), 플레이어=false(항상)
    void Init(UHealthComponent* InHealth, bool bInOnlyWhenTargeted);

protected:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // WBP에 이 이름의 HorizontalBox가 있어야 함. 하트 이미지는 코드에서 채움
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UHorizontalBox> HeartBox;

    // ===== 하트 =====
    UPROPERTY(EditAnywhere, Category = "Hearts") TObjectPtr<UTexture2D> HeartFullTexture;
    UPROPERTY(EditAnywhere, Category = "Hearts") TObjectPtr<UTexture2D> HeartEmptyTexture;
    UPROPERTY(EditAnywhere, Category = "Hearts") FVector2D HeartSize = FVector2D(22.f, 22.f);
    UPROPERTY(EditAnywhere, Category = "Hearts") float HeartSpacing = 3.f;
    UPROPERTY(EditAnywhere, Category = "Hearts") FLinearColor FullColor = FLinearColor::FromSRGBColor(FColor(0xE8, 0xE6, 0xE1));
    UPROPERTY(EditAnywhere, Category = "Hearts") FLinearColor EmptyColor = FLinearColor(1.f, 1.f, 1.f, 0.25f);

private:
    // HealthComponent의 칸 수만큼 하트 이미지를 새로 만듦
    void BuildHearts();
    // 앞에서부터 Filled칸은 채운 하트, 나머지는 빈 하트로
    void Refresh(int32 Filled);

    TWeakObjectPtr<UHealthComponent> HealthComp;
    bool bOnlyWhenTargeted = false;
    int32 LastFilled = -1;   // 칸 수가 바뀐 프레임에만 이미지를 교체하기 위한 캐시

    UPROPERTY() TArray<TObjectPtr<UImage>> Hearts;
};