// Copyright June Rhodes. MIT Licensed.

#include "MultiplayerOnlineSubsystem/Public/MOS_GameInstanceSubsystem.h"

#include "MultiplayerOnlineSubsystem/Public/Libraries/MOS_SessionsFindSessionsAsyncResult.h"
#include "MultiplayerOnlineSubsystem/Public/Libraries/MOS_Types.h"
#include "Interfaces/OnlinePartyInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystemUtils.h"
#include "DebugPlus/Public/DP_EnhancedLogging.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "WorldPartition/ContentBundle/ContentBundleLog.h"


void UMOS_GameInstanceSubsystem::GetSessionInfo(const FMOSSessionsSearchResult& SearchResult)
{
    PingInMs = SearchResult.PingInMs;
    SessionInfo = SearchResult.Session.SessionInfo->ToString();
    SessionUniqueNetID = SearchResult.Session.OwningUserId->ToString();
    SessionOwnerName = SearchResult.Session.OwningUserName;
    OpenPublicConnections = SearchResult.Session.NumOpenPublicConnections;
    OpenPrivateConnections = SearchResult.Session.NumOpenPrivateConnections;
}

void UMOS_GameInstanceSubsystem::ExecuteSessionsFindSessions(UMOS_SessionsFindSessionsAsyncResult *Result)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        Result->OnResult(false, TArray<FMOSSessionsSearchResult>(), TEXT("Online subsystem is not available."));
        return;
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    if (!UserId.IsValid())
    {
        Result->OnResult(false, TArray<FMOSSessionsSearchResult>(), TEXT("The local user is not signed in."));
        return;
    }

    // Get the session interface, if the online subsystem supports it.
    auto Session = OSS->GetSessionInterface();
    if (!Session.IsValid())
    {
        Result->OnResult(
            false,
            TArray<FMOSSessionsSearchResult>(),
            TEXT("Online subsystem does not support sessions."));
        return;
    }

    // Construct the search object.
    auto SearchObject = MakeShared<FOnlineSessionSearch>();
    SearchObject->QuerySettings.SearchParams.Empty();
    SearchObject->QuerySettings.SearchParams.Add(SEARCH_LOBBIES, FOnlineSessionSearchParam(true, EOnlineComparisonOp::Equals));
    SearchObject->QuerySettings.Set(SEARCH_KEYWORDS, FString("MOSSession"), EOnlineComparisonOp::Equals);
    
    /*search for non-listening sessions*/
 //   SearchObject->QuerySettings.Set(FName(TEXT("__EOS_bListening")), false , EOnlineComparisonOp::Equals);
    /*Show lobbies that are full*/
    SearchObject->QuerySettings.SearchParams.Add(FName(TEXT("minslotsavailable")), FOnlineSessionSearchParam((int64)0L, EOnlineComparisonOp::GreaterThanEquals));
    
    // Register an event so we can receive the query outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = Session->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateWeakLambda(
            this, [this, Session, CallbackHandle, ResultWk = TSoftObjectPtr<UMOS_SessionsFindSessionsAsyncResult>(Result), SearchObject](bool bCallbackWasSuccessful)
            {
                // Check if this callback is for us.
                if (SearchObject->SearchState != EOnlineAsyncTaskState::Failed && SearchObject->SearchState != EOnlineAsyncTaskState::Done)
                {
                    // This callback isn't for our call.
                    return;
                }

                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return if the read failed.
                if (SearchObject->SearchState == EOnlineAsyncTaskState::Failed)
                {
                    ResultWk->OnResult(false, TArray<FMOSSessionsSearchResult>(), TEXT("Session search failed."));
                    Session->ClearOnFindSessionsCompleteDelegate_Handle(*CallbackHandle);
                    return;
                }
                
                SessionResults.Empty();
                CachedFindSessionResults.Empty();
                
                for (const auto &Row : SearchObject->SearchResults)
                {
                    if (Row.Session.SessionInfo.IsValid())
                    {
                        FOnlineSessionSearchResult Entry;
                        Entry.Session = Row.Session;
                        Entry.PingInMs = Row.PingInMs;

                        SessionResults.Add(Entry);
                        
                        FMOSSessionsSearchResult CachedEntry;
                        CachedEntry.Session = Row.Session;
                        CachedEntry.PingInMs = Row.PingInMs;
                        
                        CachedFindSessionResults.Add(CachedEntry);
                    }
                }

                // Return the results.
                ResultWk->OnResult(true, CachedFindSessionResults, TEXT(""));

                // Unregister this callback since we've handled the call we care about.
                Session->ClearOnFindSessionsCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Search for all sessions.
    if (!Session->FindSessions(this->LocalUserNum, SearchObject))
    {
        Result->OnResult(true, TArray<FMOSSessionsSearchResult>(), TEXT("FindSessions call failed to start."));
        Session->ClearOnFindSessionsCompleteDelegate_Handle(*CallbackHandle);
    }
}

void UMOS_GameInstanceSubsystem::ExecuteSessionsStartListenServer(int32 InAvailableSlots)
{
    // We need somewhere to store session settings between the main menu and multiplayer map, so that when CreateSession
    // is called on the multiplayer map, it can retrieve the settings that were used to start the listen server. In this
    // example we use the game instance.
    ensureMsgf(InAvailableSlots > 0, TEXT("InAvailableSlots must be greater than 0"));
    
    auto *CurrentWorld = this->GetWorld();
    if (IsValid(CurrentWorld))
    {
       StartListenServer(InAvailableSlots);
    }
}

void UMOS_GameInstanceSubsystem::StartListenServer(int32 InAvailableSlots)
{
    if (GEngine == nullptr)
    {
        return;
    }
    // Listen for the map start.

    WorldInitHandle = FWorldDelegates::OnWorldInitializedActors.AddUObject(this, &UMOS_GameInstanceSubsystem::OnMapListening);

    // Figure out the world context from the online subsystem.
    DP_LOG(MOSGameInstanceSubsystem, Log, "Starting listen server");
    
    ensureMsgf(ListenLevel != "", TEXT("You must set the ListenLevel via SetTravelParameters(). Function is BlueprintCallable."));
    ensureMsgf(IsValid(TravelGameMode), TEXT("You must set the TravelGameMode via SetTravelParameters(). Function is BlueprintCallable."));

    
    //Get the level path
    FString LevelPath = ListenLevel.ToString();
    // Get the GameMode class path
    FString GameModePath = TravelGameMode->GetPathName();
    // Setting the travel URL to the multiplayer map.
    FString LevelOptions = FString::Printf(TEXT("listen?Game=%s&MaxPlayers=%d"), *GameModePath, InAvailableSlots);
    // Start the listen server by browsing to the multiplayer map.
 
    UGameplayStatics::OpenLevel(GetWorld(), *LevelPath, true, LevelOptions);
    //GEngine->SetClientTravel(this->GetWorld(), *TravelCommand, TRAVEL_Absolute);
}

void UMOS_GameInstanceSubsystem::OnMapListening(const UWorld::FActorsInitializedParams &ActorInitParams)
{
    ENetMode NetMode =  ActorInitParams.World->GetNetMode();
    UNetDriver *NetDriver = ActorInitParams.World->GetNetDriver();
    if (!IsValid(NetDriver))
    {
        FWorldDelegates::OnWorldInitializedActors.Remove(WorldInitHandle);
        return;
    }

    if (NetDriver->GetNetMode() != ENetMode::NM_ListenServer)
    {
        FWorldDelegates::OnWorldInitializedActors.Remove(WorldInitHandle);
        return;
    }

    DP_LOG(MOSGameInstanceSubsystem, Log, "Successfully started listen server");
    FWorldDelegates::OnWorldInitializedActors.Remove(WorldInitHandle);
    WorldInitHandle.Reset();

    ExecuteSessionsCreateSession(MOSSessionName, AsyncResult);
}

void UMOS_GameInstanceSubsystem::ExecuteSessionsCreateSession(FName SessionName, UMOS_AsyncResult *Result)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        Result->OnResult(false, TEXT("Online subsystem is not available."));
        return;
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    if (!UserId.IsValid())
    {
        Result->OnResult(false, TEXT("The local user is not signed in."));
        return;
    }

    // Get the session interface, if the online subsystem supports it.
    auto Session = OSS->GetSessionInterface();
    if (!Session.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support sessions."));
        return;
    }

    // Create the session settings.
    FOnlineSessionSettings SessionSettings;
    SessionSettings.NumPublicConnections = SessionAvailableSlots;
    SessionSettings.NumPrivateConnections = 0;
    SessionSettings.bShouldAdvertise = true;
    SessionSettings.bAllowJoinInProgress = true;
    SessionSettings.bIsLANMatch = false;
    SessionSettings.bIsDedicated = false;
    SessionSettings.bUsesStats = false;
    SessionSettings.bAllowInvites = true;
    SessionSettings.bUsesPresence = true;
    SessionSettings.bAllowJoinViaPresence = true;
    SessionSettings.bAllowJoinViaPresenceFriendsOnly = false;
    SessionSettings.bAntiCheatProtected = false;
    SessionSettings.bUseLobbiesIfAvailable = true; //OSS->GetSubsystemName().IsEqual(STEAM_SUBSYSTEM);
    SessionSettings.bUseLobbiesVoiceChatIfAvailable = false;
    SessionSettings.BuildUniqueId = 0;
    SessionSettings.Settings.Add(FName(TEXT("SessionSetting")), FOnlineSessionSetting(TEXT("SettingValue"), EOnlineDataAdvertisementType::ViaOnlineService));
    SessionSettings.Set(SEARCH_KEYWORDS, FString("MOSSession"), EOnlineDataAdvertisementType::ViaOnlineService);
    
    // Register an event so we can receive the create outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = Session->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateWeakLambda(
            this, [Session, CallbackHandle, ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result), SessionName]
            (FName CallbackSessionName, bool bCallbackWasSuccessful)
            {
                // Check if this callback is for us.
                if (!SessionName.IsEqual(CallbackSessionName))
                {
                    // This callback isn't for our call.
                    return;
                }

                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return the results.
                ResultWk->OnResult(
                    bCallbackWasSuccessful,
                    bCallbackWasSuccessful ? TEXT("") : TEXT("Create session operation failed."));

                // Unregister this callback since we've handled the call we care about.
                Session->ClearOnCreateSessionCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Create the session.
    if (!Session->CreateSession(this->LocalUserNum, SessionName, SessionSettings))
    {
        Result->OnResult(true, TEXT("CreateSession call failed to start."));
        Session->ClearOnCreateSessionCompleteDelegate_Handle(*CallbackHandle);
    }
}

void UMOS_GameInstanceSubsystem::OnCreateSessionComplete(const FName InSessionName, const bool bWasSuccessful)
{
    DP_LOG(MOSGameInstanceSubsystem, Log, "Create Session: %hs", bWasSuccessful ? "Success" : "Failed");
    DP_LOG(MOSGameInstanceSubsystem, Log, "Session Name: %s", *InSessionName.ToString());
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
         DP_LOG(MOSGameInstanceSubsystem, Warning, "Online subsystem is not available.");
        return;
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    if (!UserId.IsValid())
    {
         DP_LOG(MOSGameInstanceSubsystem, Warning, "The local user is not signed in.");
        return;
    }

    // Get the session interface, if the online subsystem supports it.
    auto Session = OSS->GetSessionInterface();
    if (!Session.IsValid())
    {
         DP_LOG(MOSGameInstanceSubsystem, Warning, "Online subsystem does not support sessions.");
        return;
    }
    
    if(OSS)
    {
        if (const TSharedPtr<IOnlineSession>(SessionPtr) = OSS->GetSessionInterface())
        {
            auto *GameMode = this->GetWorld()->GetAuthGameMode();
            if (GameMode == nullptr)
            {
                DP_LOG(MOSGameInstanceSubsystem, Warning, "GameMode is not valid.");
                return;
            }

            //Get the level path
            FString LevelPath = TravelLevel.ToString();
            
            GetWorld()->ServerTravel(LevelPath, true);
        }
    }
    DP_LOG(MOSGameInstanceSubsystem, Log, "OnCreateSession Completed.");
}

void UMOS_GameInstanceSubsystem::ExecuteSessionsJoinSession(
    const FMOSSessionsSearchResult &SearchResult,
    FName SessionName,
    UMOS_AsyncResult *Result)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        Result->OnResult(false, TEXT("Online subsystem is not available."));
        return;
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    if (!UserId.IsValid())
    {
        Result->OnResult(false, TEXT("The local user is not signed in."));
        return;
    }

    // Get the session interface, if the online subsystem supports it.
    auto Session = OSS->GetSessionInterface();
    if (!Session.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support sessions."));
        return;
    }

    FOnlineSessionSearchResult SelectedSession;
     for (auto CurrentSession : SessionResults)
     {
         if (SearchResult.Session.GetSessionIdStr() == CurrentSession.Session.GetSessionIdStr())
         {
             SelectedSession.Session = SearchResult.Session;
             SelectedSession.PingInMs = SearchResult.PingInMs;
             break;
         }
     }

    // Register an event so we can receive the outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = Session->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateWeakLambda(
        this,
        [this, Session, CallbackHandle, ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result), SessionName](
            FName CallbackSessionName,
            EOnJoinSessionCompleteResult::Type CallbackResult) {
            // Check if this callback is for us.
            if (!SessionName.IsEqual(CallbackSessionName))
            {
                // This callback isn't for our call.
                return;
            }

            // Make sure the result callback is still valid.
            if (!ResultWk.IsValid())
            {
                return;
            }

            // If we're not successful, return now.
            if (CallbackResult != EOnJoinSessionCompleteResult::Success)
            {
                FString ErrorMessage;
                switch (CallbackResult)
                {
                case EOnJoinSessionCompleteResult::SessionIsFull:
                    ErrorMessage = TEXT("SessionIsFull");
                    break;
                case EOnJoinSessionCompleteResult::SessionDoesNotExist:
                    ErrorMessage = TEXT("SessionDoesNotExist");
                    break;
                case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress:
                    ErrorMessage = TEXT("CouldNotRetrieveAddress");
                    break;
                case EOnJoinSessionCompleteResult::AlreadyInSession:
                    ErrorMessage = TEXT("AlreadyInSession");
                    break;
                case EOnJoinSessionCompleteResult::UnknownError:
                    ErrorMessage = TEXT("UnknownError");
                    break;
                case EOnJoinSessionCompleteResult::Success:
                default:
                    break;
                }
                ResultWk->OnResult(false, ErrorMessage);
                Session->ClearOnJoinSessionCompleteDelegate_Handle(*CallbackHandle);
                return;
            }

            // @note: Not all subsystems require this, but after we join the session, now connect to the game server.
            FString ConnectInfo;
            Session->GetResolvedConnectString(SessionName, ConnectInfo);
            GEngine->SetClientTravel(this->GetWorld(), *ConnectInfo, TRAVEL_Absolute);

            // Return the results.
            ResultWk->OnResult(true, TEXT(""));

            // Unregister this callback since we've handled the call we care about.
            Session->ClearOnJoinSessionCompleteDelegate_Handle(*CallbackHandle);
        }));

    // Join the session.
    if (!Session->JoinSession(this->LocalUserNum, SessionName, SelectedSession))
    {
        Result->OnResult(true, TEXT("JoinSession call failed to start."));
        Session->ClearOnJoinSessionCompleteDelegate_Handle(*CallbackHandle);
    }
}

void UMOS_GameInstanceSubsystem::OnJoinSessionComplete(FName InSessionName, EOnJoinSessionCompleteResult::Type Result)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr){return;}
    // Get the session interface, if the online subsystem supports it.
    auto Session = OSS->GetSessionInterface();
    if (!Session.IsValid()){return;}
    // Get the Player Controller.
    
    FString ConnectInfo;
    Session->GetResolvedConnectString(InSessionName, ConnectInfo);
    DP_LOG(MOSGameInstanceSubsystem, Log, "ConnectionInfo: %s", *ConnectInfo);

    GEngine->SetClientTravel(this->GetWorld(), *ConnectInfo, TRAVEL_Absolute);
}

void UMOS_GameInstanceSubsystem::ExecuteSessionsJoinSessionWithParty(const FMOSSessionsSearchResult &SearchResult, FName SessionName, UMOS_AsyncResult *Result)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        Result->OnResult(false, TEXT("Online subsystem is not available."));
        return;
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    if (!UserId.IsValid())
    {
        Result->OnResult(false, TEXT("The local user is not signed in."));
        return;
    }

    // Get the session interface, if the online subsystem supports it.
    auto Session = OSS->GetSessionInterface();
    if (!Session.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support sessions."));
        return;
    }
    
    if (!SearchResult.Session.SessionInfo.IsValid())
    {
        Result->OnResult(false, TEXT("No such search result exists."));
        return;
    }

    FOnlineSessionSearchResult SelectedSession;
    for (auto CurrentSession : SessionResults)
    {
        if (SearchResult.Session.GetSessionIdStr() == CurrentSession.Session.GetSessionIdStr())
        {
            SelectedSession.Session = SearchResult.Session;
            SelectedSession.PingInMs = SearchResult.PingInMs;
            break;
        }
    }

    // Register an event so we can receive the outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = Session->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateWeakLambda(
        this,
        [this,
         Session,
         Identity,
         UserId,
         CallbackHandle,
         ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result),
         SessionName](FName CallbackSessionName, EOnJoinSessionCompleteResult::Type CallbackResult) {
            // Check if this callback is for us.
            if (!SessionName.IsEqual(CallbackSessionName))
            {
                // This callback isn't for our call.
                return;
            }

            // Make sure the result callback is still valid.
            if (!ResultWk.IsValid())
            {
                return;
            }

            // If we're not successful, return now.
            if (CallbackResult != EOnJoinSessionCompleteResult::Success)
            {
                FString ErrorMessage;
                switch (CallbackResult)
                {
                case EOnJoinSessionCompleteResult::SessionIsFull:
                    ErrorMessage = TEXT("SessionIsFull");
                    break;
                case EOnJoinSessionCompleteResult::SessionDoesNotExist:
                    ErrorMessage = TEXT("SessionDoesNotExist");
                    break;
                case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress:
                    ErrorMessage = TEXT("CouldNotRetrieveAddress");
                    break;
                case EOnJoinSessionCompleteResult::AlreadyInSession:
                    ErrorMessage = TEXT("AlreadyInSession");
                    break;
                case EOnJoinSessionCompleteResult::UnknownError:
                    ErrorMessage = TEXT("UnknownError");
                    break;
                case EOnJoinSessionCompleteResult::Success:
                default:
                    break;
                }
                ResultWk->OnResult(false, ErrorMessage);
                Session->ClearOnJoinSessionCompleteDelegate_Handle(*CallbackHandle);
                return;
            }

            // Get the online subsystem.
            auto CallbackOSS = Online::GetSubsystem(this->GetWorld());
            if (CallbackOSS == nullptr)
            {
                ResultWk->OnResult(false, TEXT("Online subsystem is not available."));
                return;
            }

            // Get the party interface, if the online subsystem supports it.
            auto PartySystem = CallbackOSS->GetPartyInterface();
            if (!PartySystem.IsValid())
            {
                ResultWk->OnResult(false, TEXT("Online subsystem does not support parties."));
                return;
            }

            // Get the session ID.
            FString SessionId;
            auto *CurrentSession = Session->GetNamedSession(CallbackSessionName);
            if (CurrentSession != nullptr)
            {
                SessionId = CurrentSession->GetSessionIdStr();
            }

            // Find the first party that the user is also a leader of.
            TArray<TSharedRef<const FOnlinePartyId>> PartyIds;
            PartySystem->GetJoinedParties(*UserId, PartyIds);
            TSharedPtr<const FOnlinePartyId> SelectedPartyId;
            for (const auto &PartyId : PartyIds)
            {
                if (PartySystem->IsMemberLeader(*UserId, *PartyId, *UserId))
                {
                    SelectedPartyId = PartyId;
                    break;
                }
            }
            if (!SelectedPartyId.IsValid())
            {
                // Just join the game server - there's no party.
                FString ConnectInfo;
                Session->GetResolvedConnectString(SessionName, ConnectInfo);
                GEngine->SetClientTravel(this->GetWorld(), *ConnectInfo, TRAVEL_Absolute);
                ResultWk->OnResult(true, TEXT(""));
                Session->ClearOnJoinSessionCompleteDelegate_Handle(*CallbackHandle);
                return;
            }

            // Update the party data.
            CachedLastPartySessionId = SessionId;
            auto PartyData = MakeShared<FOnlinePartyData>(*PartySystem->GetPartyData(*UserId, *SelectedPartyId, NAME_None));
            PartyData->SetAttribute(TEXT("JoinSessionIdFromParty"), FVariantData(SessionId));
            PartySystem->UpdatePartyData(*UserId, *SelectedPartyId, NAME_None, *PartyData);

            // @note: Not all subsystems require this, but after we join the session, now connect to the game server.
            FString ConnectInfo;
            Session->GetResolvedConnectString(SessionName, ConnectInfo);
            GEngine->SetClientTravel(this->GetWorld(), *ConnectInfo, TRAVEL_Absolute);

            // Return the results.
            ResultWk->OnResult(true, TEXT(""));

            // Unregister this callback since we've handled the call we care about.
            Session->ClearOnJoinSessionCompleteDelegate_Handle(*CallbackHandle);
        }));

    // Join the session.
    if (!Session->JoinSession(this->LocalUserNum, SessionName, SelectedSession))
    {
        Result->OnResult(true, TEXT("JoinSession call failed to start."));
        Session->ClearOnJoinSessionCompleteDelegate_Handle(*CallbackHandle);
    }
}

FString UMOS_GameInstanceSubsystem::GetSessionId(FName SessionName) const
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        return TEXT("");
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    if (!UserId.IsValid())
    {
        return TEXT("");
    }

    // Get the session interface, if the online subsystem supports it.
    auto Session = OSS->GetSessionInterface();
    if (!Session.IsValid())
    {
        return TEXT("");
    }

    // Return the session ID.
    auto *NamedSession = Session->GetNamedSession(SessionName);
    if (NamedSession == nullptr)
    {
        return TEXT("");
    }
    return NamedSession->GetSessionIdStr();
}

TArray<FUniqueNetIdRepl> UMOS_GameInstanceSubsystem::GetSessionRegisteredPlayerIds(FName SessionName) const
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        return TArray<FUniqueNetIdRepl>();
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    if (!UserId.IsValid())
    {
        return TArray<FUniqueNetIdRepl>();
    }

    // Get the session interface, if the online subsystem supports it.
    auto Session = OSS->GetSessionInterface();
    if (!Session.IsValid())
    {
        return TArray<FUniqueNetIdRepl>();
    }

    // Get the named session.
    auto *NamedSession = Session->GetNamedSession(SessionName);
    if (NamedSession == nullptr)
    {
        return TArray<FUniqueNetIdRepl>();
    }

    // Return the registered session players.
    TArray<FUniqueNetIdRepl> PlayerIds;
    for (const auto &PlayerId : NamedSession->RegisteredPlayers)
    {
        PlayerIds.Add(PlayerId);
    }
    return PlayerIds;
}

void UMOS_GameInstanceSubsystem::ExecuteSessionsStartSession(FName SessionName, UMOS_AsyncResult *Result)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        Result->OnResult(false, TEXT("Online subsystem is not available."));
        return;
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    if (!UserId.IsValid())
    {
        Result->OnResult(false, TEXT("The local user is not signed in."));
        return;
    }

    // Get the session interface, if the online subsystem supports it.
    auto Session = OSS->GetSessionInterface();
    if (!Session.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support sessions."));
        return;
    }

    // Register an event so we can receive the outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle =
        Session->AddOnStartSessionCompleteDelegate_Handle(FOnStartSessionCompleteDelegate::CreateWeakLambda(
            this,
            [Session, CallbackHandle, ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result), SessionName](
                FName CallbackSessionName,
                bool bCallbackWasSuccessful) {
                // Check if this callback is for us.
                if (!SessionName.IsEqual(CallbackSessionName))
                {
                    // This callback isn't for our call.
                    return;
                }

                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return the results.
                ResultWk->OnResult(
                    bCallbackWasSuccessful,
                    bCallbackWasSuccessful ? TEXT("") : TEXT("Start session operation failed."));

                // Unregister this callback since we've handled the call we care about.
                Session->ClearOnStartSessionCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Start the session.
    if (!Session->StartSession(SessionName))
    {
        Result->OnResult(true, TEXT("StartSession call failed to start."));
        Session->ClearOnStartSessionCompleteDelegate_Handle(*CallbackHandle);
    }
}

void UMOS_GameInstanceSubsystem::ExecuteSessionsEndSession(FName SessionName, UMOS_AsyncResult *Result)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        Result->OnResult(false, TEXT("Online subsystem is not available."));
        return;
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    if (!UserId.IsValid())
    {
        Result->OnResult(false, TEXT("The local user is not signed in."));
        return;
    }

    // Get the session interface, if the online subsystem supports it.
    auto Session = OSS->GetSessionInterface();
    if (!Session.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support sessions."));
        return;
    }

    // Register an event so we can receive the outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = Session->AddOnEndSessionCompleteDelegate_Handle(FOnEndSessionCompleteDelegate::CreateWeakLambda(
        this,
        [Session, CallbackHandle, ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result), SessionName](
            FName CallbackSessionName,
            bool bCallbackWasSuccessful) {
            // Check if this callback is for us.
            if (!SessionName.IsEqual(CallbackSessionName))
            {
                // This callback isn't for our call.
                return;
            }

            // Make sure the result callback is still valid.
            if (!ResultWk.IsValid())
            {
                return;
            }

            // Return the results.
            ResultWk->OnResult(
                bCallbackWasSuccessful,
                bCallbackWasSuccessful ? TEXT("") : TEXT("End session operation failed."));

            // Unregister this callback since we've handled the call we care about.
            Session->ClearOnEndSessionCompleteDelegate_Handle(*CallbackHandle);
        }));

    // End the session.
    if (!Session->EndSession(SessionName))
    {
        Result->OnResult(true, TEXT("EndSession call failed to start."));
        Session->ClearOnEndSessionCompleteDelegate_Handle(*CallbackHandle);
    }
}

void UMOS_GameInstanceSubsystem::ExecuteSessionsOpenInviteUI(FName SessionName)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        return;
    }

    // Get the external UI interface, if the online subsystem supports it.
    auto ExternalUI = OSS->GetExternalUIInterface();
    if (!ExternalUI.IsValid())
    {
        return;
    }

    // Open the invite UI.
    ExternalUI->ShowInviteUI(this->LocalUserNum, SessionName);
}

void UMOS_GameInstanceSubsystem::ExecuteSessionsDestroySession(FName SessionName, UMOS_AsyncResult *Result)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        Result->OnResult(false, TEXT("Online subsystem is not available."));
        return;
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    if (!UserId.IsValid())
    {
        Result->OnResult(false, TEXT("The local user is not signed in."));
        return;
    }

    // Get the session interface, if the online subsystem supports it.
    auto Session = OSS->GetSessionInterface();
    if (!Session.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support sessions."));
        return;
    }

    // Register an event so we can receive the outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle =
        Session->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateWeakLambda(
            this,
            [Session, CallbackHandle, ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result), SessionName](
                FName CallbackSessionName,
                bool bCallbackWasSuccessful) {
                // Check if this callback is for us.
                if (!SessionName.IsEqual(CallbackSessionName))
                {
                    // This callback isn't for our call.
                    return;
                }

                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return the results.
                ResultWk->OnResult(
                    bCallbackWasSuccessful,
                    bCallbackWasSuccessful ? TEXT("") : TEXT("Destroy session operation failed."));

                // Unregister this callback since we've handled the call we care about.
                Session->ClearOnDestroySessionCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Destroy the session.
    if (!Session->DestroySession(SessionName))
    {
        Result->OnResult(true, TEXT("DestroySession call failed to start."));
        Session->ClearOnDestroySessionCompleteDelegate_Handle(*CallbackHandle);
    }
}

void UMOS_GameInstanceSubsystem::ExecuteSessionsRegisterPlayer(
    FName SessionName,
    const FUniqueNetIdRepl &PlayerId,
    bool bWasFromInvite,
    UMOS_AsyncResult *Result)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        Result->OnResult(false, TEXT("Online subsystem is not available."));
        return;
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    if (!UserId.IsValid())
    {
        Result->OnResult(false, TEXT("The local user is not signed in."));
        return;
    }

    // Get the session interface, if the online subsystem supports it.
    auto Session = OSS->GetSessionInterface();
    if (!Session.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support sessions."));
        return;
    }

    // Register an event so we can receive the outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle =
        Session->AddOnRegisterPlayersCompleteDelegate_Handle(FOnRegisterPlayersCompleteDelegate::CreateWeakLambda(
            this,
            [Session,
             CallbackHandle,
             ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result),
             SessionName](FName CallbackSessionName, const TArray<FUniqueNetIdRef> &, bool bCallbackWasSuccessful) {
                // Check if this callback is for us.
                if (!SessionName.IsEqual(CallbackSessionName))
                {
                    // This callback isn't for our call.
                    return;
                }

                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return the results.
                ResultWk->OnResult(
                    bCallbackWasSuccessful,
                    bCallbackWasSuccessful ? TEXT("") : TEXT("RegisterPlayer operation failed."));

                // Unregister this callback since we've handled the call we care about.
                Session->ClearOnRegisterPlayersCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Register the player.
    if (!Session->RegisterPlayer(SessionName, *PlayerId, bWasFromInvite))
    {
        Result->OnResult(true, TEXT("RegisterPlayer call failed to start."));
        Session->ClearOnRegisterPlayersCompleteDelegate_Handle(*CallbackHandle);
    }
}

void UMOS_GameInstanceSubsystem::OnRegisterPlayersComplete(FName SessionName, const TArray<FUniqueNetIdRef> &PlayerIds, bool bWasSuccessful)
{
    for (const auto& PlayerId : PlayerIds)
    {
        DP_LOG(MOSGameInstanceSubsystem, Log, "Registered PlayerId: %s", *PlayerId->ToString());
    }
}

void UMOS_GameInstanceSubsystem::ExecuteSessionsUnregisterPlayer(
    FName SessionName,
    const FUniqueNetIdRepl &PlayerId,
    UMOS_AsyncResult *Result)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        Result->OnResult(false, TEXT("Online subsystem is not available."));
        return;
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    if (!UserId.IsValid())
    {
        Result->OnResult(false, TEXT("The local user is not signed in."));
        return;
    }

    // Get the session interface, if the online subsystem supports it.
    auto Session = OSS->GetSessionInterface();
    if (!Session.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support sessions."));
        return;
    }

    // Register an event so we can receive the outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle =
        Session->AddOnUnregisterPlayersCompleteDelegate_Handle(FOnUnregisterPlayersCompleteDelegate::CreateWeakLambda(
            this,
            [Session,
             CallbackHandle,
             ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result),
             SessionName](FName CallbackSessionName, const TArray<FUniqueNetIdRef> &, bool bCallbackWasSuccessful) {
                // Check if this callback is for us.
                if (!SessionName.IsEqual(CallbackSessionName))
                {
                    // This callback isn't for our call.
                    return;
                }

                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return the results.
                ResultWk->OnResult(
                    bCallbackWasSuccessful,
                    bCallbackWasSuccessful ? TEXT("") : TEXT("UnregisterPlayer operation failed."));

                // Unregister this callback since we've handled the call we care about.
                Session->ClearOnUnregisterPlayersCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Register the player.
    if (!Session->UnregisterPlayer(SessionName, *PlayerId))
    {
        Result->OnResult(true, TEXT("UnregisterPlayer call failed to start."));
        Session->ClearOnUnregisterPlayersCompleteDelegate_Handle(*CallbackHandle);
    }
}

void UMOS_GameInstanceSubsystem::ExecuteSessionsReturnToMainMenu()
{
    // Move back to the main menu map.
    GEngine->SetClientTravel(this->GetWorld(), TEXT("/Game/OO/Maps/MainMenu"), TRAVEL_Absolute);
}
