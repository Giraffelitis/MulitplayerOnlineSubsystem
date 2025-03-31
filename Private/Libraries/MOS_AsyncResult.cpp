// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerOnlineSubsystem/Public/Libraries/MOS_AsyncResult.h"

void UMOS_AsyncResult::OnResult(bool bWasSuccessful, const FString& ErrorMessage)
{
	if (!this->bDidCallback)
	{
		this->bDidCallback = true;
		if (this->NativeCallback.IsBound())
		{
			this->NativeCallback.Execute(bWasSuccessful, ErrorMessage);
		}
	}
}
