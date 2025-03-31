// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MOS_GetAvatarAsyncResult.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERONLINESUBSYSTEM_API UMOS_GetAvatarAsyncResult : public UObject
{
	GENERATED_BODY()

public:
	typedef const TSoftObjectPtr<UTexture> &ResultType;

	typedef TDelegate<void(bool, const TSoftObjectPtr<UTexture> &, FString)> FNativeCallback;
	bool bDidCallback;
	FNativeCallback NativeCallback;

	UFUNCTION(BlueprintCallable, Category = "Callbacks")
	void OnResult(bool bWasSuccessful, const TSoftObjectPtr<UTexture> &Result, const FString &ErrorMessage);
};

