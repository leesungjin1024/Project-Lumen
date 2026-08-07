# Enhanced Input 설정 가이드 (4-2-1 기준)

Content/Lumen/ 하위에 `Input` 폴더를 하나 추가해서 그 안에 만드는 걸 권장합니다.
(`Content/Lumen/Input/`)

## 1단계 — Input Action(IA) 에셋 만들기

콘텐츠 브라우저 `Content/Lumen/Input/` 폴더에서 **우클릭 → Input → Input Action**으로 아래 8개를 생성합니다.

| 이름 | Value Type | 이유 |
|---|---|---|
| `IA_Move` | Axis2D (Vector2D) | 이동은 앞뒤/좌우 두 축이 동시에 필요 |
| `IA_Aim` | Axis2D (Vector2D) | 게임패드 오른쪽 스틱용 (마우스는 아래 "중요 주의사항" 참고) |
| `IA_Fire` | Digital (bool) | 누르는 동안(홀드) 발사, 발열 누적 |
| `IA_Shield` | Digital (bool) | 누르는 동안(홀드) 방패 들기 |
| `IA_Dash` | Digital (bool) | 순간 입력(트리거) |
| `IA_Interact` | Digital (bool) | 순간 입력(트리거) |
| `IA_Pause` | Digital (bool) | 순간 입력(트리거) |

**색 전환은 별도로 3개(또는 4개)를 만드는 걸 권장**합니다 — 1/2/3 키가 "어떤 색으로 바꿔라"라는 각기 다른 값을 전달해야 하는데, Enhanced Input에서 이걸 하나의 Axis 값으로 억지로 합치면 오히려 코드가 복잡해집니다.

| 이름 | Value Type |
|---|---|
| `IA_SelectRed` | Digital (bool) |
| `IA_SelectBlue` | Digital (bool) |
| `IA_SelectGreen` | Digital (bool) |

(마우스 휠 순환은 나중에 필요해지면 `IA_CycleColor`(Axis1D)로 추가 — 지금은 숫자 키만으로 충분)

## 2단계 — 각 IA의 Trigger 설정 (에셋을 더블클릭해서 열면 나오는 Triggers 항목)

기본값(Trigger 없음)이면 "누르고 있는 동안 값이 흐르고, 떼면 0"이 되는 게 기본 동작이라 대부분은 **아무 것도 추가하지 않아도 됩니다.** 다만 아래는 명시적으로 지정하는 걸 권장:

- `IA_Dash`, `IA_Interact`, `IA_Pause`, `IA_SelectRed/Blue/Green`: **Pressed** 트리거 추가 (누르는 "순간"에 한 번만 발동, 홀드 방지)
- `IA_Fire`, `IA_Shield`: 트리거 없음(기본값) 유지 — C++에서 `Started`(누른 순간)/`Completed`(뗀 순간) 두 이벤트를 각각 바인딩해서 "누르는 동안 지속" 로직을 구현 (5-1-1 발열 누적, 5-4-1 방패 게이지 소모와 직결)

## 3단계 — Input Mapping Context(IMC) 만들기

콘텐츠 브라우저에서 **우클릭 → Input → Input Mapping Context** → 이름 `IMC_Default`

더블클릭해서 열고, `+` 버튼으로 위에서 만든 IA들을 하나씩 추가한 뒤 키를 매핑합니다.

### IA_Move (WASD) — Modifier 조합이 필요한 유일한 항목
UE5 표준 방식으로, 키 하나당 축 하나만 담당하게 한 뒤 Modifier로 합칩니다.

| 키 | Modifiers |
|---|---|
| W | Swizzle Input Axis Values (YXZ) |
| S | Swizzle Input Axis Values (YXZ) + Negate |
| A | Negate |
| D | (Modifier 없음) |

(게임패드는 그냥 `Left Thumbstick 2D-Axis`를 추가하면 별도 Modifier 없이 바로 Vector2D로 들어옵니다)

### IA_Aim (게임패드 전용)
- `Right Thumbstick 2D-Axis` 추가, Modifier는 Dead Zone 정도만 (기본값으로 충분)

### IA_Fire
- `Left Mouse Button` 추가
- `Gamepad Right Trigger` 추가 (아날로그 값이지만 Digital 액션에 매핑하면 자동으로 임계값 처리됨)

### IA_Shield
- `Right Mouse Button` 추가
- `Gamepad Left Trigger` 추가

### IA_Dash
- `Space Bar` 추가
- `Gamepad Face Button Right`(B/Circle) 추가

### IA_SelectRed / IA_SelectBlue / IA_SelectGreen
- 각각 `1`, `2`, `3` 키 추가
- 게임패드는 4-2-1에 "LB/RB 순환"으로 정해뒀으니, 지금은 생략하고 숫자 키부터 동작 확인 후 추가해도 무방

### IA_Interact
- `E` 키 추가
- `Gamepad Face Button Bottom`(X/Square) 추가

### IA_Pause
- `Escape` 키 추가
- `Gamepad Special Right`(Start/Options 버튼) 추가

## 4단계 — 중요 주의사항: 마우스 조준은 IA_Aim으로 처리하지 않습니다

4-2-1에서 "조준 방향 = 마우스 위치(방향)"이라고 했는데, 이건 마우스가 "얼마나 움직였는가"(델타)가 아니라 **"화면의 어디를 가리키고 있는가"(커서 위치)**를 봐야 하는 문제라, Enhanced Input의 축 입력으로 처리하기보다 다음 방식이 맞습니다:

- 매 프레임 `GetHitResultUnderCursor()` (또는 캐릭터 높이의 가상 평면에 `DeprojectScreenToWorld` + 평면 교차 계산)로 "마우스가 가리키는 월드 좌표"를 구함
- 그 좌표와 캐릭터 위치를 빼서 방향 벡터를 얻음 → 이게 조준/방패 각도(4-2)가 됨
- 이 계산은 Tick에서 폴링하는 로직이라 Input Action 바인딩이 필요 없음 — **`IA_Aim`은 게임패드 오른쪽 스틱 전용**이고, 마우스는 이 계산 결과를 그대로 같은 변수(예: `CurrentAimDirection`)에 넣어주면 이후 로직(조준/방패 각도 갱신)은 마우스든 패드든 동일하게 동작합니다

## 5단계 — 코드에서 Mapping Context 등록

캐릭터 또는 PlayerController의 `BeginPlay`(혹은 `PossessedBy`)에서:

```cpp
if (APlayerController* PC = Cast<APlayerController>(GetController()))
{
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
    {
        Subsystem->AddMappingContext(IMC_Default, 0); // 0 = 우선순위
    }
}
```

`SetupPlayerInputComponent`에서는 `UEnhancedInputComponent`로 캐스팅한 뒤 `BindAction`으로 각 IA를 연결합니다 (예: `IA_Fire`는 `ETriggerEvent::Started`와 `ETriggerEvent::Completed` 두 번 바인딩).

---

여기까지가 1주차(6-1) 범위입니다. 실제 `BindAction` 코드와 캐릭터 클래스(`.h`/`.cpp`)는 카메라 세팅과 같이 묶어서 다음에 요청 주시면 작성해드리겠습니다.
