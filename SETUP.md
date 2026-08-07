# LUMEN 프로젝트 착수 가이드 (0주차 → 1주차)

이 폴더(`.gitignore`, `.gitattributes`, `Content/Lumen/...`)는 기획서 9-4, 9-4-1 결정을
그대로 옮겨둔 스캐폴드입니다. UE 에디터는 이 컨테이너에서 실행할 수 없으니,
아래 순서대로 **본인 PC에서** 직접 진행하면서 이 파일들을 그대로 가져다 쓰면 됩니다.

## 0단계 — Git LFS 설치 확인 (한 번만)
```
git lfs install
```
설치가 안 돼 있으면 https://git-lfs.com 에서 받아서 설치.

## 1단계 — UE 5.5.4 설치 (Epic Games Launcher)
- Epic Games Launcher → Unreal Engine → Library → **5.5.4** 설치
- 정확히 5.5.4인지 확인 (4-3 결정 사항 — 마이너 버전 달라도 대부분 호환되지만, 나중에 버전 문제로 헤매지 않으려면 맞춰두는 게 안전)

## 2단계 — 프로젝트 생성 (Third Person, C++ 템플릿)
- Epic Games Launcher → Unreal Engine → **Games → Third Person** 템플릿 선택
- **반드시 C++ 선택** (Blueprint 아님) — 4-3 결정: "대부분의 로직은 C++"
- 프로젝트 이름: `Lumen` (가제, 9-4-1 폴더 구조의 `/Content/Lumen/`과 이름을 맞추기 위함)
- 저장 위치: 앞으로 git 저장소가 될 폴더 (예: `C:\Projects\Lumen` 또는 `~/Projects/Lumen`)

## 3단계 — 이 스캐폴드 파일 옮기기
프로젝트가 생성되면 `Lumen.uproject`가 있는 최상위 폴더에 아래를 복사:
- `.gitignore` → 프로젝트 최상위에 덮어쓰기
- `.gitattributes` → 프로젝트 최상위에 덮어쓰기
- `Content/Lumen/` 폴더 전체 → 프로젝트의 기존 `Content/` 폴더 **안에** 병합
  (템플릿이 만든 `Content/ThirdPerson/` 등 기존 폴더는 그대로 두고, `Content/Lumen/`을 나란히 추가)

## 4단계 — Git 저장소 초기화
프로젝트 최상위 폴더(`Lumen.uproject`가 있는 곳)에서:
```
git init
git lfs install
git add .gitattributes .gitignore
git add .
git commit -m "chore: 프로젝트 셋업, 9-4/9-4-1 폴더·LFS 규칙 적용"
```
- `git add .` 전에 `.gitattributes`를 먼저 add하는 이유: LFS 필터가 먼저 적용된 상태에서
  나머지 파일을 add해야 `.uasset` 등이 처음부터 LFS로 올바르게 추적됨

## 5단계 — Enhanced Input 에셋 생성 (4-2-1)
콘텐츠 브라우저에서 `Content/Lumen/` 하위에 Input 관련 에셋 생성 (기획서 4-2-1 참고):
- `IA_Move`, `IA_Aim`, `IA_Fire`, `IA_Shield`, `IA_Dash`, `IA_ColorSwitch`, `IA_Interact`, `IA_Pause`
- `IMC_Default` (Input Mapping Context)
→ 이건 에디터 GUI 조작이라 여기서 대신 만들어드릴 수 없습니다. 4-2-1의 키 배치표를 그대로 옮기면 됩니다.

## 6단계 — 카메라 시스템 (4-1)
- Third Person 템플릿의 스프링암+카메라 컴포넌트를 고정 아이소메트릭으로 변경
  (회전 로직 제거, 카메라 고도/거리/FOV는 4-1 "프로토타입 확인 필요 사항"에 따라 실험적으로 조정)

## 이후 순서
여기까지가 6-1 표의 **1주차** 범위입니다. 완료되면 2주차(반사 시스템 스파이크)로 넘어가면 됩니다 —
2주차부터는 실제 C++ 클래스 작성이 필요하니, 코드는 그때그때 요청해주시면 파일로 만들어드리겠습니다.

## 참고: 이 컨테이너가 할 수 없는 것
- UE 에디터 실행, 컴파일, 실제 동작 확인은 전부 본인 PC에서 진행해야 합니다 (6-1-1 "Claude 협업의 근본적 한계" 참고)
- C++ 클래스는 요청하시면 `.h`/`.cpp` 파일로 작성해드릴 수 있지만, 실제 컴파일 성공 여부는 직접 확인 후
  에러 메시지를 알려주시면 그걸 보고 같이 고치는 방식으로 진행하면 됩니다
