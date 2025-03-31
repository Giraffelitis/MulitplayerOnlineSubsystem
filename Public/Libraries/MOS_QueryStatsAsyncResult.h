// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MOS_QueryStatsAsyncResult.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERONLINESUBSYSTEM_API UMOS_QueryStatsAsyncResult : public UObject
{
	GENERATED_BODY()

public:
	typedef TDelegate<void(bool, const TArray<FOSSStatsStatState> &, FString)> FNativeCallback;
	bool bDidCallback;
	FNativeCallback NativeCallback;

	typedef const TArray<FOSSStatsStatState> &ResultType;

	UFUNCTION(BlueprintCallable, Category = "Callbacks")
	void OnResult(
		bool bWasSuccessful,
		const TArray<FOSSStatsStatState> &Results,
		const FString &ErrorMessage);
};
