// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "MOS_QueryLeaderboardsAsyncResult.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERONLINESUBSYSTEM_API UMOS_QueryLeaderboardsAsyncResult : public UObject
{
	GENERATED_BODY()

public:
	typedef TDelegate<void(bool, const TArray<FMOSLeaderboardsLeaderboardEntry> &, FString)> FNativeCallback;
	bool bDidCallback;
	FNativeCallback NativeCallback;

	typedef const TArray<FMOSLeaderboardsLeaderboardEntry> &ResultType;

	UFUNCTION(BlueprintCallable, Category = "Callbacks")
	void OnResult(
		bool bWasSuccessful,
		const TArray<FMOSLeaderboardsLeaderboardEntry> &Results,
		const FString &ErrorMessage);
};
