// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerOnlineSubsystem/Public/Libraries/MOS_QueryStatsAsyncResult.h"

void UMOS_QueryStatsAsyncResult::OnResult(
	bool bWasSuccessful,
	const TArray<FMOSStatsStatState> &Results,
	const FString &ErrorMessage)
{
	if (!this->bDidCallback)
	{
		this->bDidCallback = true;
		this->NativeCallback.Execute(bWasSuccessful, Results, ErrorMessage);
	}
}