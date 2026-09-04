// Copyright Epic Games, Inc. All Rights Reserved.

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
            "AIModule",        // ← 추가: BehaviorTree, BTTaskNode, BTService, BlackboardComponent, AAIController
            "GameplayTasks",   // ← 추가: BehaviorTreeComponent가 GameplayTasksComponent를 상속
            "ProceduralMeshComponent", //추가: 조준선
			"UMG", "Slate", "SlateCore", // ui 
        });
	}
}
