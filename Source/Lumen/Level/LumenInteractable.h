// ILumenInteractable — E키 상호작용 대상 공용 인터페이스. 문(ALumenDoorActor) / 회전 거울(AMirrorActor) / 퍼즐 리셋(APuzzleResetActor)
// 흐름: 대상이 InteractionZone 오버랩에서 ALumenCharacter::AddInteractable/RemoveInteractable 호출 → 캐릭터가 매 틱
//   CanInteract()인 것 중 가장 가까운 대상 위(GetPromptLocation)에 프롬프트(GetInteractPrompt) 표시 → E 누름 Interact / 뗌 InteractReleased / 누른 채 일시정지 InteractCanceled

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LumenInteractable.generated.h"

class ALumenCharacter;

UINTERFACE(MinimalAPI)
class ULumenInteractable : public UInterface { GENERATED_BODY() };

class LUMEN_API ILumenInteractable
{
	GENERATED_BODY()

public:
	// E를 누른 순간
	virtual void Interact(ALumenCharacter* Interactor) = 0;

	// E를 뗀 순간 — 홀드 판정이 필요한 대상(회전 거울)만 구현
	virtual void InteractReleased(ALumenCharacter* Interactor) {}

	// 누른 채 일시정지 등으로 상호작용이 중단됨 — 홀드 상태를 정리
	virtual void InteractCanceled() {}

	// false면 선택 대상에서 빠지고 프롬프트도 안 뜸 (예: 이미 풀린 퍼즐의 리셋, 움직이는 중인 문)
	virtual bool CanInteract() const { return true; }

	// 프롬프트 문구 — 상태에 따라 바뀌어도 됨 (문: 열기/닫기)
	virtual FText GetInteractPrompt() const { return FText::FromString(TEXT("[E]")); }

	// 프롬프트를 띄울 월드 위치 (보통 메시 위쪽)
	virtual FVector GetPromptLocation() const = 0;
};