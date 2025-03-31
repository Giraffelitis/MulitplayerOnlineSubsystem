// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MOS_QueryAchievementsAsyncResult.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERONLINESUBSYSTEM_API UMOS_QueryAchievementsAsyncResult : public UObject
{
	GENERATED_BODY()

public:
	typedef TDelegate<void(bool, const TArray<FMOSAchievementsAchievementState> &, FString)> FNativeCallback;
	bool bDidCallback;
	FNativeCallback NativeCallback;

	typedef const TArray<FMOSAchievementsAchievementState> &ResultType;

	UFUNCTION(BlueprintCallable, Category = "Callbacks")
	void OnResult(
		bool bWasSuccessful,
		const TArray<FMOSAchievementsAchievementState> &Results,
		const FString &ErrorMessage);
};
