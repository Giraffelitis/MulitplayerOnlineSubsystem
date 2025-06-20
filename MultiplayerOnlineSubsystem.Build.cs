// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MultiplayerOnlineSubsystem : ModuleRules
{
	public MultiplayerOnlineSubsystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] 
		{ 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput",
			
		});
		
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreOnline",
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			"VoiceChat",
			"SlateCore",
			"Json",

		});
		
#if UE_5_5_OR_LATER
		PublicDefinitions.Add("REDPOINT_EXAMPLE_UE_5_5_OR_LATER=1");
#else
		PublicDefinitions.Add("REDPOINT_EXAMPLE_UE_5_5_OR_LATER=0");
#endif
	}
}
