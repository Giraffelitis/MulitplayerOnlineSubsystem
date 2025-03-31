// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerOnlineSusbsystem/Public/Libraries/MOS_QueryAchievementsAsyncResult.h"

void UMOS_QueryAchievementsAsyncResult::OnResult(
	bool bWasSuccessful,
	const TArray<FOSSAchievementsAchievementState> &Results,
	const FString &ErrorMessage)
{
	if (!this->bDidCallback)
	{
		this->bDidCallback = true;
		this->NativeCallback.Execute(bWasSuccessful, Results, ErrorMessage);
	}
}
