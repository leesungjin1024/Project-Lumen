#include "LumenInteractPromptWidget.h"
#include "Components/TextBlock.h"

void ULumenInteractPromptWidget::SetPromptText(const FText& InText)
{
	if (PromptText)
	{
		PromptText->SetText(InText);
	}
}
