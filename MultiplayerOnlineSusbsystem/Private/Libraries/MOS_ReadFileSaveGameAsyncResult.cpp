// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerOnlineSusbsystem/Public/Libraries/MOS_ReadFileSaveGameAsyncResult.h"

void UMOS_ReadFileSaveGameAsyncResult::OnResult(bool bWasSuccessful, double Result, const FString &ErrorMessage)
{
	if (!this->bDidCallback)
	{
		this->bDidCallback = true;
		this->NativeCallback.Execute(bWasSuccessful, Result, ErrorMessage);
	}
}