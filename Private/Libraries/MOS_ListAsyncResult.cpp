// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerOnlineSusbsystem/Public/Libraries/MOS_ListAsyncResult.h"

void UMOS_ListAsyncResult::OnResult(
	bool bWasSuccessful,
	const TArray<FOSSInterfaceListEntry> &Results,
	const FString &ErrorMessage)
{
	if (!this->bDidCallback)
	{
		this->bDidCallback = true;
		this->NativeCallback.Execute(bWasSuccessful, Results, ErrorMessage);
	}
}