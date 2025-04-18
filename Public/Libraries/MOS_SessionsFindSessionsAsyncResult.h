// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MOS_SessionsFindSessionsAsyncResult.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERONLINESUBSYSTEM_API UMOS_SessionsFindSessionsAsyncResult : public UObject
{
	GENERATED_BODY()
	
public:
	typedef TDelegate<void(bool, const TArray<FMOSSessionsSearchResult> &, FString)> FNativeCallback;
	bool bDidCallback;
	FNativeCallback NativeCallback;

	typedef const TArray<FMOSSessionsSearchResult> &ResultType;

	UFUNCTION(BlueprintCallable, Category = "Callbacks")
	void OnResult(bool bWasSuccessful, const TArray<FMOSSessionsSearchResult> &Results, const FString &ErrorMessage);
	
};
