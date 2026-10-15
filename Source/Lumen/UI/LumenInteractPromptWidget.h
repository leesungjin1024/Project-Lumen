// ULumenInteractPromptWidget — E 상호작용 프롬프트 위젯의 C++ 부모(WBP_InteractPrompt). 이름이 "PromptText"인 TextBlock 필요.
// 연결: ALumenCharacter::UpdateInteractPrompt, ILumenInteractable::GetInteractPrompt

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LumenInteractPromptWidget.generated.h"

class UTextBlock;

UCLASS(Abstract)
class LUMEN_API ULumenInteractPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 프롬프트 문구 변경 (예: "[E] 열기")
	void SetPromptText(const FText& InText);

protected:
	// WBP에 같은 이름의 TextBlock이 없으면 위젯 컴파일 에러로 바로 알려줌
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PromptText;
};
