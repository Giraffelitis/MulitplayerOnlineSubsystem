// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerOnlineSusbsystem/Public/Libraries/MOS_ReadFileStringAsyncResult.h"

void UMOS_ReadFileStringAsyncResult::OnResult(
	bool bWasSuccessful,
	const FString &Result,
	const FString &ErrorMessage)
{
	if (!this->bDidCallback)
	{
		this->bDidCallback = true;
		this->NativeCallback.Execute(bWasSuccessful, Result, ErrorMessage);
	}
}