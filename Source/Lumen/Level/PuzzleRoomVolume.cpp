#include "PuzzleRoomVolume.h"
#include "Components/BoxComponent.h"
#include "LumenCharacter.h"
#include "LumenDoorActor.h"
#include "PuzzleSensorActor.h"
#include "Kismet/GameplayStatics.h"
#include "LumenCollisionChannels.h" // ECC_ProjectileTrace
#include "MirrorActor.h"
#include "Components/CapsuleComponent.h"
#include "Sound/SoundBase.h"

namespace
{
	// TArray<TObjectPtr<T>>::Contains에 const 포인터를 넘기는 비교를 피하려고 직접 비교
	bool ArrayHasSensor(const TArray<TObjectPtr<APuzzleSensorActor>>& Arr, const APuzzleSensorActor* Sensor)
	{
		for (const TObjectPtr<APuzzleSensorActor>& Elem : Arr)
		{
			if (Elem.Get() == Sensor) return true;
		}
		return false;
	}
}

APuzzleRoomVolume::APuzzleRoomVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	EntryTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("EntryTrigger"));
	RootComponent = EntryTrigger;
	EntryTrigger->SetCollisionProfileName(TEXT("Trigger"));
	// Trigger 프리셋이 커스텀 채널(ProjectileTrace) 기본값 Block을 물려받으므로 명시적으로 Ignore (ACombatRoomVolume과 동일)
	EntryTrigger->SetCollisionResponseToChannel(ECC_ProjectileTrace, ECR_Ignore);
	EntryTrigger->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	bOverrideCameraPitch = true;
}

void APuzzleRoomVolume::BeginPlay()
{
	Super::BeginPlay();
	EntryTrigger->OnComponentBeginOverlap.AddDynamic(this, &APuzzleRoomVolume::OnEntryOverlap);
	EntryTrigger->OnComponentEndOverlap.AddDynamic(this, &APuzzleRoomVolume::OnEntryEndOverlap);

	// ── 설정 검증 ──
	int32 ValidAndSensors = 0;
	for (int32 i = 0; i < Sensors.Num(); ++i)
	{
		if (Sensors[i]) { ++ValidAndSensors; continue; }
		UE_LOG(LogTemp, Warning, TEXT("[PuzzleRoom] %s: Sensors[%d]이 비어 있음 (None) — 무시함"), *GetName(), i);
	}
	if (ValidAndSensors > 0 && ExitDoors.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PuzzleRoom] %s: Sensors는 있는데 ExitDoors가 비어 있음 — 전부 켜도 열 문이 없음"), *GetName());
	}

	for (int32 r = 0; r < DoorRules.Num(); ++r)
	{
		const FPuzzleDoorRule& Rule = DoorRules[r];
		int32 ValidRuleSensors = 0;
		for (const APuzzleSensorActor* S : Rule.RequiredSensors) { if (S) ++ValidRuleSensors; }

		if (ValidRuleSensors == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[PuzzleRoom] %s: 규칙 %s에 센서가 없음 — 이 규칙은 절대 완료되지 않음"), *GetName(), *GetRuleLabel(r));
		}
		if (Rule.DoorsToOpen.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[PuzzleRoom] %s: 규칙 %s에 열 문이 없음 (순서 단계용이면 무시해도 됨)"), *GetName(), *GetRuleLabel(r));
		}
		if (r == 0 && Rule.bRequirePreviousRule)
		{
			UE_LOG(LogTemp, Warning, TEXT("[PuzzleRoom] %s: 첫 번째 규칙의 bRequirePreviousRule은 무시됨"), *GetName());
		}
	}

	// ── 센서 자동 연결 ── Sensors + 규칙의 센서 전부 (센서마다 OwningRoom을 손으로 지정할 필요 없음)
	TArray<APuzzleSensorActor*> AllSensors;
	GatherAllSensors(AllSensors);
	for (APuzzleSensorActor* Sensor : AllSensors)
	{
		if (Sensor->OwningRoom && Sensor->OwningRoom != this)
		{
			UE_LOG(LogTemp, Warning, TEXT("[PuzzleRoom] %s: 센서 %s의 OwningRoom이 다른 방(%s)으로 지정돼 있었음 — 이 방으로 덮어씀"),
				*GetName(), *Sensor->GetName(), *Sensor->OwningRoom->GetName());
		}
		Sensor->OwningRoom = this;
	}

	if (AllSensors.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PuzzleRoom] %s: 연결된 센서가 없음 — 이 방은 절대 클리어되지 않음"), *GetName());
	}
	UE_LOG(LogTemp, Log, TEXT("[PuzzleRoom] %s: 센서 %d개 / AND형 문 %d개 / 규칙 %d개"),
		*GetName(), AllSensors.Num(), ExitDoors.Num(), DoorRules.Num());
}

void APuzzleRoomVolume::OnEntryOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	// 캡슐로만 판정 — 캐릭터의 다른 컴포넌트(메시 등)가 따로 들어오고 나가면서
	// CurrentRoom이 엉뚱한 시점에 지워지는 것 방지
	ALumenCharacter* Player = Cast<ALumenCharacter>(OtherActor);
	if (!Player || OtherComp != Player->GetCapsuleComponent())
	{
		return;
	}

	Player->SetCurrentRoom(this); // 사망 시 이 방을 리셋하기 위함
	ApplyRoomView(Player, EntryTrigger->Bounds.GetBox()); // 카메라 + 방 밖 마스크(bMaskOutsideRoom) 함께 처리
}

void APuzzleRoomVolume::OnEntryEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ALumenCharacter* Player = Cast<ALumenCharacter>(OtherActor);
	if (!Player || OtherComp != Player->GetCapsuleComponent())
	{
		return;
	}

	Player->ClearCurrentRoomIfMatches(this);
	ClearRoomView(Player); // 이 방이 현재 카메라 방일 때만 카메라/마스크 해제
}

// ===== 조회 =====

void APuzzleRoomVolume::GatherAllSensors(TArray<APuzzleSensorActor*>& OutSensors) const
{
	OutSensors.Reset();
	for (APuzzleSensorActor* S : Sensors)
	{
		if (S) OutSensors.AddUnique(S);
	}
	for (const FPuzzleDoorRule& Rule : DoorRules)
	{
		for (APuzzleSensorActor* S : Rule.RequiredSensors)
		{
			if (S) OutSensors.AddUnique(S);
		}
	}
}

bool APuzzleRoomVolume::ContainsSensor(const APuzzleSensorActor* Sensor) const
{
	if (!Sensor) return false;
	if (ArrayHasSensor(Sensors, Sensor)) return true;
	for (const FPuzzleDoorRule& Rule : DoorRules)
	{
		if (ArrayHasSensor(Rule.RequiredSensors, Sensor)) return true;
	}
	return false;
}

FString APuzzleRoomVolume::GetRuleLabel(int32 Index) const
{
	if (DoorRules.IsValidIndex(Index) && !DoorRules[Index].RuleName.IsNone())
	{
		return FString::Printf(TEXT("[%d:%s]"), Index, *DoorRules[Index].RuleName.ToString());
	}
	return FString::Printf(TEXT("[%d]"), Index);
}

bool APuzzleRoomVolume::IsRuleLocked(int32 Index) const
{
	if (Index <= 0 || !DoorRules.IsValidIndex(Index)) return false;
	return DoorRules[Index].bRequirePreviousRule && !DoorRules[Index - 1].bCompleted;
}

bool APuzzleRoomVolume::CanSensorActivate(const APuzzleSensorActor* Sensor) const
{
	if (!Sensor) return false;

	// AND형 센서는 항상 켜질 수 있음
	if (ArrayHasSensor(Sensors, Sensor)) return true;

	// 규칙 센서: 이 센서를 쓰는 규칙 중 하나라도 잠겨 있지 않으면 켜질 수 있음
	bool bInAnyRule = false;
	for (int32 r = 0; r < DoorRules.Num(); ++r)
	{
		if (!ArrayHasSensor(DoorRules[r].RequiredSensors, Sensor)) continue;
		bInAnyRule = true;
		if (!IsRuleLocked(r)) return true;
	}

	// 어떤 규칙에도 없는 센서는 막지 않음
	return !bInAnyRule;
}

bool APuzzleRoomVolume::AreAllSensorsActivated() const
{
	// 빈 칸(None)은 건너뜀 — 안 그러면 빈 칸 하나 때문에 문이 영원히 안 열림
	int32 ValidCount = 0;
	for (const APuzzleSensorActor* Sensor : Sensors)
	{
		if (!Sensor)
		{
			continue;
		}
		if (!Sensor->IsActivated())
		{
			return false;
		}
		++ValidCount;
	}
	return ValidCount > 0; // 센서가 하나도 없으면 false
}

bool APuzzleRoomVolume::IsPuzzleSolved() const
{
	int32 ValidAndSensors = 0;
	for (const APuzzleSensorActor* S : Sensors) { if (S) ++ValidAndSensors; }

	const bool bUsesAnd = ValidAndSensors > 0;
	const bool bUsesRules = DoorRules.Num() > 0;

	if (!bUsesAnd && !bUsesRules)
	{
		return false; // 아무것도 설정 안 된 방
	}
	if (bUsesAnd && !AreAllSensorsActivated())
	{
		return false;
	}
	for (const FPuzzleDoorRule& Rule : DoorRules)
	{
		if (!Rule.bCompleted) return false;
	}
	return true;
}

// ===== 진행 =====

void APuzzleRoomVolume::NotifySensorActivated()
{
	// ── 규칙형: 앞에서부터 검사 → 규칙 하나가 완료되면 바로 다음(순서형) 규칙도 같은 호출 안에서 검사됨 ──
	bool bAnyRuleCompleted = false;
	for (int32 r = 0; r < DoorRules.Num(); ++r)
	{
		FPuzzleDoorRule& Rule = DoorRules[r];
		if (Rule.bCompleted || IsRuleLocked(r)) continue;

		int32 Valid = 0, Activated = 0;
		for (const APuzzleSensorActor* S : Rule.RequiredSensors)
		{
			if (!S) continue;
			++Valid;
			if (S->IsActivated()) ++Activated;
		}
		if (Valid == 0 || Activated < Valid) continue;

		Rule.bCompleted = true;
		bAnyRuleCompleted = true;
		UE_LOG(LogTemp, Log, TEXT("[PuzzleRoom] %s: 규칙 %s 완료 — 문 %d개 개방"), *GetName(), *GetRuleLabel(r), Rule.DoorsToOpen.Num());
		for (ALumenDoorActor* Door : Rule.DoorsToOpen)
		{
			if (Door) Door->Open();
		}
	}

	// ── AND형 ──
	int32 Activated = 0, Total = 0;
	for (const APuzzleSensorActor* Sensor : Sensors)
	{
		if (!Sensor) continue;
		++Total;
		if (Sensor->IsActivated()) ++Activated;
	}
	if (Total > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[PuzzleRoom] %s: AND형 센서 %d / %d 활성화"), *GetName(), Activated, Total);
	}

	if (AreAllSensorsActivated())
	{
		UE_LOG(LogTemp, Log, TEXT("[PuzzleRoom] %s: AND형 클리어 — 문 %d개 개방"), *GetName(), ExitDoors.Num());
		for (ALumenDoorActor* Door : ExitDoors)
		{
			if (Door) Door->Open();
		}
	}

	// ── 사운드 ── 전체 클리어면 ClearSound 한 번, 아니면 규칙 완료음
	if (!bSolvedNotified && IsPuzzleSolved())
	{
		bSolvedNotified = true;
		UE_LOG(LogTemp, Log, TEXT("[PuzzleRoom] %s: 퍼즐 전체 클리어"), *GetName());
		if (ClearSound)
		{
			UGameplayStatics::PlaySound2D(this, ClearSound);
		}
	}
	else if (bAnyRuleCompleted && RuleCompleteSound)
	{
		UGameplayStatics::PlaySound2D(this, RuleCompleteSound);
	}
}

void APuzzleRoomVolume::ResetRoom()
{
	// 이미 클리어한 방은 리셋하지 않음 — 퍼즐을 푼 뒤 그 방에서 죽어도 문이 다시 닫히지 않게
	if (IsPuzzleSolved())
	{
		UE_LOG(LogTemp, Log, TEXT("[PuzzleRoom] %s: 이미 클리어됨 — 리셋 생략"), *GetName());
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[PuzzleRoom] %s: 사망으로 리셋 — 센서/규칙 초기화"), *GetName());

	bool bAnyAndWasActivated = false;
	for (const APuzzleSensorActor* S : Sensors)
	{
		if (S && S->IsActivated()) bAnyAndWasActivated = true;
	}

	// 센서 전부 끄기 (AND형 + 규칙형)
	TArray<APuzzleSensorActor*> AllSensors;
	GatherAllSensors(AllSensors);
	for (APuzzleSensorActor* Sensor : AllSensors)
	{
		Sensor->Deactivate();
	}

	// 완료됐던 규칙의 문 닫기 + 규칙 상태 초기화
	for (FPuzzleDoorRule& Rule : DoorRules)
	{
		if (Rule.bCompleted)
		{
			for (ALumenDoorActor* Door : Rule.DoorsToOpen)
			{
				if (Door) Door->Close();
			}
		}
		Rule.bCompleted = false;
	}

	// 이미 아무도 안 맞춘 상태였으면(퍼즐 시작 전 죽음) 문을 굳이 다시 닫을 필요 없음
	if (bAnyAndWasActivated)
	{
		for (ALumenDoorActor* Door : ExitDoors)
		{
			if (Door) Door->Close();
		}
	}
}

void APuzzleRoomVolume::ResetMirrors()
{
	for (AMirrorActor* Mirror : Mirrors)
	{
		if (Mirror) Mirror->ResetToInitialRotation();
	}
}
