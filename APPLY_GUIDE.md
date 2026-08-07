# LumenCharacter 적용 가이드

## 1. 파일 위치
프로젝트의 `Source/<프로젝트명>/` 폴더에 `LumenCharacter.h`, `LumenCharacter.cpp`를 넣습니다.
(예: `Source/ProjectLumen/LumenCharacter.h`)

**API 매크로 이름 확인 필수**: `LumenCharacter.h`의 `LUMEN_API`는 플레이스홀더입니다.
`Source/<프로젝트명>/<프로젝트명>Character.h`(템플릿이 만든 기존 캐릭터 클래스)를 열어서
`class PROJECTLUMEN_API AProjectLumenCharacter : ...` 같은 줄에서 실제 매크로 이름을 확인하고,
`LumenCharacter.h`의 `LUMEN_API`를 그 이름으로 바꿔주세요.

## 2. Build.cs에 EnhancedInput 모듈 추가 (누락하면 컴파일 에러)
`Source/<프로젝트명>/<프로젝트명>.Build.cs` 파일을 열어서
`PublicDependencyModuleNames`에 `"EnhancedInput"`이 있는지 확인합니다. 없으면 추가:

```csharp
PublicDependencyModuleNames.AddRange(new string[] {
    "Core", "CoreUObject", "Engine", "InputCore",
    "EnhancedInput"   // 이 줄이 없으면 InputActionValue.h 등을 못 찾아서 컴파일 에러
});
```

## 3. 컴파일
- Visual Studio(또는 Rider)에서 빌드 → 에러 없이 컴파일되면 성공
- 에러가 나면 에러 메시지 전체를 캡처해서 알려주시면 같이 확인하겠습니다

## 4. 에디터에서 블루프린트로 감싸기
C++ 클래스는 직접 레벨에 배치해서 쓰기보다, 블루프린트로 한 번 감싸는 게 표준 방식입니다.
- 콘텐츠 브라우저 → `Content/Lumen/Characters/` → 우클릭 → Blueprint Class
- 부모 클래스로 `LumenCharacter` 선택 → 이름 `BP_LumenCharacter`

## 5. BP_LumenCharacter에서 Input 에셋 연결
`BP_LumenCharacter`를 열고 왼쪽 `Class Defaults` 또는 Details 패널의 **Input** 카테고리에서
지난번 만든 에셋들을 하나씩 드래그해서 연결:

| 슬롯 | 연결할 에셋 |
|---|---|
| Default Mapping Context | `IMC_Default` |
| Move Action | `IA_Move` |
| Aim Action | `IA_Aim` |
| Fire Action | `IA_Fire` |
| Shield Action | `IA_Shield` |
| Dash Action | `IA_Dash` |
| Select Red/Blue/Green Action | `IA_SelectRed` / `IA_SelectBlue` / `IA_SelectGreen` |
| Interact Action | `IA_Interact` |
| Pause Action | `IA_Pause` |

## 6. 게임모드에 기본 Pawn으로 지정
`Content/Lumen/` 하위 GameMode 블루프린트(또는 템플릿이 만든 기존 GameMode)의
Default Pawn Class를 `BP_LumenCharacter`로 지정합니다.

## 7. 실행 확인 체크리스트
- [ ] Play 눌렀을 때 카메라가 45도 회전된 고정 시점으로 보이는가 (4-1)
- [ ] WASD로 이동하는가
- [ ] 마우스를 움직이면 캐릭터가 커서 방향을 보고 도는가 (이동과 무관하게)
- [ ] 좌클릭/우클릭 시 Output Log에 `StartFire`/`StartShield` 로그가 찍히는가
- [ ] Space로 `Dash` 로그, 1/2/3으로 `SelectColor` 로그가 찍히는가

여기까지 확인되면 1주차(6-1) 범위가 끝난 것입니다. 다음은 2주차 반사 시스템 스파이크로 넘어가면 됩니다.

## 참고: 지금 카메라 각도/거리 값은 가안입니다
`CameraPitch = -55`, `CameraYaw = 45`, `CameraArmLength = 1200`은 시작점일 뿐이고,
4-1에서 "그레이박스 단계에서 실험적으로 확정"하기로 했던 값들입니다.
`BP_LumenCharacter`의 Camera 카테고리에서 에디터 중에도 바로 조정 가능하니,
실제로 플레이해보면서 화면에 방/적/투사체가 잘 보이는 값으로 맞춰보세요.
