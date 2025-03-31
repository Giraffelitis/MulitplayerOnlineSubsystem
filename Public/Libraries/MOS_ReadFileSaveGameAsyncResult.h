// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MOS_ReadFileSaveGameAsyncResult.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERONLINESUBSYSTEM_API UMOS_ReadFileSaveGameAsyncResult : public UObject
{
	GENERATED_BODY()

public:
	typedef TDelegate<void(bool, double, FString)> FNativeCallback;
	bool bDidCallback;
	FNativeCallback NativeCallback;

	typedef double ResultType;

	UFUNCTION(BlueprintCallable, Category = "Callbacks")
	void OnResult(bool bWasSuccessful, double Result, const FString &ErrorMessage);
};
