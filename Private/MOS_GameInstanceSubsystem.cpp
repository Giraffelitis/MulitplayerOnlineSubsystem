//  Copyright Grumpy Giraffe Games. All Rights Reserved.


#include "MultiplayerOnlineSubsystem/Public/MOS_GameInstanceSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "GameFramework/GameModeBase.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MultiplayerOnlineSubsystem/Public/Libraries/MOS_SessionsFindSessionsAsyncResult.h"

typedef TMap<FString, int32> TMap_FString_int32;

DEFINE_LOG_CATEGORY(OOGameInstance);

void UMOS_GameInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{	
	Super::Initialize(Collection);

	// Get the online subsystem.
	auto OSS = Online::GetSubsystem(this->GetWorld());
	if (OSS == nullptr){return;}
    
	// Get the identity interface.
	auto Identity = OSS->GetIdentityInterface();
	if (!Identity.IsValid()) {return;}
    
	Identity->OnLoginCompleteDelegates->AddUObject(this, &UMOS_GameInstanceSubsystem::OnLoginCompleted);
	Identity->OnLogoutCompleteDelegates->AddUObject(this, &UMOS_GameInstanceSubsystem::OnLogoutCompleted);

	auto SessionPtr = OSS->GetSessionInterface();
	if (SessionPtr == nullptr){return;}

	SessionPtr->OnCreateSessionCompleteDelegates.AddUObject(this, &ThisClass::OnCreateSessionComplete);
	SessionPtr->OnFindSessionsCompleteDelegates.AddUObject(this, &ThisClass::OnFindSessionsComplete);
	SessionPtr->OnJoinSessionCompleteDelegates.AddUObject(this, &ThisClass::OnJoinSessionComplete);

	FindSessionsAsyncResult = NewObject<UMOS_SessionsFindSessionsAsyncResult>();
	AsyncResult = NewObject<UMOS_AsyncResult>();
}

void UMOS_GameInstanceSubsystem::RaiseFriendsOnFriendsChange()
{
}

void UMOS_GameInstanceSubsystem::RaisePartiesOnPartiesStateChanged()
{
}

void UMOS_GameInstanceSubsystem::SetTravelParameters(const FName LevelName, const TSubclassOf<AGameModeBase> GameMode)
{
	TravelLevel = LevelName;
	TravelGameMode = GameMode;
}