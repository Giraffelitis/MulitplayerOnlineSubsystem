// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerOnlineSusbsystem/Public/Libraries/MOS_SessionsFindSessionsAsyncResult.h"

#include "MultiplayerOnlineSusbsystem/Public/Libraries/MOS_Types.h"

void UMOS_SessionsFindSessionsAsyncResult::OnResult(bool bWasSuccessful, const TArray<FOSSSessionsSearchResult> &Results,	const FString &ErrorMessage)
{
	if (!this->bDidCallback)
	{
		this->bDidCallback = true;
		if (this->NativeCallback.IsBound())
		{
			this->NativeCallback.Execute(bWasSuccessful, Results, ErrorMessage);
		}
	}
}
