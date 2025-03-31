// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MOS_ListAsyncResult.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERONLINESUBSYSTEM_API UMOS_ListAsyncResult : public UObject
{
	GENERATED_BODY()

public:
	typedef TDelegate<void(bool, const TArray<FMOSInterfaceListEntry> &, FString)> FNativeCallback;
	bool bDidCallback;
	FNativeCallback NativeCallback;

	typedef const TArray<FMOSInterfaceListEntry> &ResultType;

	UFUNCTION(BlueprintCallable, Category = "Callbacks")
	void OnResult(
		bool bWasSuccessful,
		const TArray<FMOSInterfaceListEntry> &Results,
		const FString &ErrorMessage);
};
