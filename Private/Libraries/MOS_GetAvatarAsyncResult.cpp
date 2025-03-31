// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerOnlineSubsystem/Public/Libraries/MOS_GetAvatarAsyncResult.h"

void UMOS_GetAvatarAsyncResult::OnResult(
	bool bWasSuccessful,
	const TSoftObjectPtr<UTexture> &Results,
	const FString &ErrorMessage)
{
	UE_LOG(	LogTemp, Verbose, TEXT("%p -> UMOS_GetAvatarAsyncResult::OnResult(%s, %s, %s) -> bDidCallback=%s"),
		this, bWasSuccessful ? TEXT("true") : TEXT("false"), Results.IsValid() ? TEXT("valid texture") : TEXT("invalid texture"), *ErrorMessage,
		this->bDidCallback ? TEXT("true") : TEXT("false"));
	if (!this->bDidCallback)
	{
		this->bDidCallback = true;
		this->NativeCallback.Execute(bWasSuccessful, Results, ErrorMessage);
	}
}