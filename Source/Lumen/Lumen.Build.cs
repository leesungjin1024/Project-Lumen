// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class Lumen : ModuleRules
{
	public Lumen(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", "CoreUObject", "Engine", "InputCore", 
			"EnhancedInput", 
			"Niagara" ,
			"AIModule",                 // BehaviorTree / BTTaskNode / BTService / Blackboard / AAIController
			"GameplayTasks",            // BehaviorTreeComponent가 GameplayTasksComponent를 상속
			"ProceduralMeshComponent",  // 조준선(ULumenAimLineComponent) 메시
			"UMG", "Slate", "SlateCore", // 위젯(HUD, 메뉴, 상호작용 프롬프트)
		});

		// 모듈 폴더 아래 모든 하위 폴더를 include 경로에 추가한다.
		// 덕분에 코드에서는 #include "MirrorActor.h"처럼 파일 이름만 쓰고,
		// 파일을 다른 기능 폴더로 옮겨도 include를 고칠 필요가 없다.
		// (파일 이름은 UHT 규칙상 프로젝트 전체에서 고유하므로 충돌 없음)
		foreach (string Dir in Directory.GetDirectories(ModuleDirectory, "*", SearchOption.AllDirectories))
		{
			PublicIncludePaths.Add(Dir);
		}
	}
}
