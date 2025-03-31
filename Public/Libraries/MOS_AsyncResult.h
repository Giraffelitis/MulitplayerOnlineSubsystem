// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MOS_AsyncResult.generated.h"

UCLASS()
class MULTIPLAYERONLINESUBSYSTEM_API UMOS_AsyncResult : public UObject
{
	GENERATED_BODY()

public:
	typedef TDelegate<void(bool, FString)> FNativeCallback;
	bool bDidCallback;
	FNativeCallback NativeCallback;

	UFUNCTION(BlueprintCallable, Category = "Callbacks")
	void OnResult(bool bWasSuccessful, const FString &ErrorMessage);
};

