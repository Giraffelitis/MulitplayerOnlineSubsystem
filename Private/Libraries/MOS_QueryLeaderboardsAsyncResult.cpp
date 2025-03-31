// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerOnlineSubsystem/Public/Libraries/MOS_QueryLeaderboardsAsyncResult.h"

void UMOS_QueryLeaderboardsAsyncResult::OnResult(bool bWasSuccessful,
	const TArray<FMOSLeaderboardsLeaderboardEntry> &Results,
	const FString &ErrorMessage)
{
	if (!this->bDidCallback)
	{
		this->bDidCallback = true;
		this->NativeCallback.Execute(bWasSuccessful, Results, ErrorMessage);
	}
}
