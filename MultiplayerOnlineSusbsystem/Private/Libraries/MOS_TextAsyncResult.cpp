// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerOnlineSusbsystem/Public/Libraries/MOS_TextAsyncResult.h"

void UMOS_TextAsyncResult::OnResult(bool bWasSuccessful, const FString &Results, const FString &ErrorMessage)
{
	if (!this->bDidCallback)
	{
		this->bDidCallback = true;
		this->NativeCallback.Execute(bWasSuccessful, Results, ErrorMessage);
	}
}