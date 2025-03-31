// Copyright June Rhodes. MIT Licensed.

#include "MultiplayerOnlineSubsystem/Public/MOS_GameInstanceSubsystem.h"

#include "Interfaces/OnlineFriendsInterface.h"
#include "Interfaces/OnlinePartyInterface.h"
#include "Interfaces/OnlinePresenceInterface.h"
#include "Interfaces/OnlineUserInterface.h"
#include "OnlineSubsystemUtils.h"
#include "DebugPlus/Public/DP_EnhancedLogging.h"

void UMOS_GameInstanceSubsystem::ExecuteFriendsQueryFriends(UMOS_AsyncResult *Result)
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
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the friends interface, if the online subsystem supports it.
    auto Friends = OSS->GetFriendsInterface();
    if (!Friends.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support friends."));
        return;
    }

    // Ask the online subsystem to cache the initial list of friends. We don't return friends inside
    // ExecuteFriendsQueryFriends; we're just getting the online subsystem to cache them so that a later
    // GetFriendsCurrentFriends can return them.
    if (!Friends->ReadFriendsList(
            this->LocalUserNum,
            TEXT(""),
            FOnReadFriendsListComplete::CreateWeakLambda(
                this,
                [ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(
                     Result)](int32, bool bWasSuccessful, const FString &, const FString &ErrorStr) {
                    // Make sure the result callback is still valid.
                    if (!ResultWk.IsValid())
                    {
                        return;
                    }

                    // Return the result.
                    ResultWk->OnResult(bWasSuccessful, ErrorStr);
                })))
    {
        Result->OnResult(false, TEXT("The ReadFriendsList call failed to start."));
        return;
    }
}

TArray<FUniqueNetIdRepl> UMOS_GameInstanceSubsystem::GetFriendsCurrentFriends() const
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
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the friends interface, if the online subsystem supports it.
    auto Friends = OSS->GetFriendsInterface();
    if (!Friends.IsValid())
    {
        return TArray<FUniqueNetIdRepl>();
    }

    // Call GetFriendsList and return the user IDs of all the user's friends.
    TArray<FUniqueNetIdRepl> CurrentFriendIds;
    TArray<TSharedRef<FOnlineFriend>> CurrentFriends;
    if (!Friends->GetFriendsList(this->LocalUserNum, TEXT(""), CurrentFriends))
    {
        return TArray<FUniqueNetIdRepl>();
    }
    for (const auto &CurrentFriend : CurrentFriends)
    {
        CurrentFriendIds.Add(CurrentFriend->GetUserId());
        DP_LOG(LogTemp, Verbose, "%s", *CurrentFriend->GetRealName())
    }
    return CurrentFriendIds;
}

FMOSFriendsFriendState UMOS_GameInstanceSubsystem::GetFriendsFriendState(const FUniqueNetIdRepl &TargetUserId) const
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        return FMOSFriendsFriendState();
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the friends interface, if the online subsystem supports it.
    auto Friends = OSS->GetFriendsInterface();
    if (!Friends.IsValid())
    {
        return FMOSFriendsFriendState();
    }

    // Attempt to get the target user's friend information.
    auto TargetFriend = Friends->GetFriend(this->LocalUserNum, *TargetUserId, TEXT(""));
    if (!TargetFriend.IsValid())
    {
        return FMOSFriendsFriendState();
    }

    // Attempt to get the party ID.
    FString PartyId;
    auto Party = OSS->GetPartyInterface();
    if (Party.IsValid())
    {
        auto AdvertisedPartyId = Party->GetAdvertisedParty(*UserId, *TargetUserId, Party->GetPrimaryPartyTypeId());
        if (AdvertisedPartyId.IsValid())
        {
            PartyId = AdvertisedPartyId->GetPartyId()->ToString();
        }
    }

    // Convert the main state.
    FMOSFriendsFriendState State;
    State.Id = TargetFriend->GetUserId();
    State.DisplayName = TargetFriend->GetDisplayName();
    State.RealName = TargetFriend->GetRealName();
    switch (TargetFriend->GetInviteStatus())
    {
    case EInviteStatus::Accepted:
        State.InvitationStatus = EMOSFriendsFriendInvitationStatus::Accepted;
        break;
    case EInviteStatus::PendingInbound:
        State.InvitationStatus = EMOSFriendsFriendInvitationStatus::PendingInbound;
        break;
    case EInviteStatus::PendingOutbound:
        State.InvitationStatus = EMOSFriendsFriendInvitationStatus::PendingOutbound;
        break;
    case EInviteStatus::Blocked:
        State.InvitationStatus = EMOSFriendsFriendInvitationStatus::Blocked;
        break;
    case EInviteStatus::Suggested:
        State.InvitationStatus = EMOSFriendsFriendInvitationStatus::Suggested;
        break;
    case EInviteStatus::Unknown:
    default:
        State.InvitationStatus = EMOSFriendsFriendInvitationStatus::Unknown;
        break;
    }
    const auto &Presence = TargetFriend->GetPresence();
    State.PresenceSessionId = Presence.SessionId;
    State.PresencePartyId = PartyId;
    State.bPresenceIsOnline = Presence.bIsOnline;
    State.bPresenceIsPlaying = Presence.bIsPlaying;
    State.bPresenceIsPlayingThisGame = Presence.bIsPlayingThisGame;
    State.bPresenceIsJoinable = Presence.bIsJoinable;
    State.bPresenceHasVoiceSupport = Presence.bHasVoiceSupport;
    State.PresenceLastOnline = Presence.LastOnline;
    State.PresenceStatusString = Presence.Status.StatusStr;
    switch (Presence.Status.State)
    {
    case EOnlinePresenceState::Online:
        State.PresenceStatusState = EMOSFriendsFriendPresenceStatus::Online;
        break;
    case EOnlinePresenceState::Away:
        State.PresenceStatusState = EMOSFriendsFriendPresenceStatus::Away;
        break;
    case EOnlinePresenceState::ExtendedAway:
        State.PresenceStatusState = EMOSFriendsFriendPresenceStatus::ExtendedAway;
        break;
    case EOnlinePresenceState::DoNotDisturb:
        State.PresenceStatusState = EMOSFriendsFriendPresenceStatus::DoNotDisturb;
        break;
    case EOnlinePresenceState::Chat:
        State.PresenceStatusState = EMOSFriendsFriendPresenceStatus::Chat;
        break;
    case EOnlinePresenceState::Offline:
    default:
        State.PresenceStatusState = EMOSFriendsFriendPresenceStatus::Offline;
        break;
    }
    for (const auto &KV : Presence.Status.Properties)
    {
        State.PresenceStatusProperties.Add(KV.Key, KV.Value.ToString());
    }

    // Get a list of attribute keys to fetch from the friend.
    TArray<FString> Keys{
        TEXT("id"),
        TEXT("ready"),
        TEXT("productUserId"),
        TEXT("displayName"),
        TEXT("prefDisplayName"),
        TEXT("deletable"),
        TEXT("eosSynthetic.primaryFriend.subsystemName"),
        TEXT("eosSynthetic.preferredFriend.subsystemName"),
    };
    FString SubsystemNames;
    TArray<FString> SubsystemNamesArray;
    TargetFriend->GetUserAttribute(TEXT("eosSynthetic.subsystemNames"), SubsystemNames);
    SubsystemNames.ParseIntoArray(SubsystemNamesArray, TEXT(","));
    for (const auto &SubsystemName : SubsystemNamesArray)
    {
        Keys.Add(FString::Printf(TEXT("eosSynthetic.friend.%s.id"), *SubsystemName));
        Keys.Add(FString::Printf(TEXT("eosSynthetic.friend.%s.realName"), *SubsystemName));
        Keys.Add(FString::Printf(TEXT("eosSynthetic.friend.%s.displayName"), *SubsystemName));
    }
    FString ExternalAccountTypes;
    TArray<FString> ExternalAccountTypesArray;
    TargetFriend->GetUserAttribute(TEXT("externalAccountTypes"), ExternalAccountTypes);
    ExternalAccountTypes.ParseIntoArray(ExternalAccountTypesArray, TEXT(","));
    for (const auto &ExternalAccount : ExternalAccountTypesArray)
    {
        Keys.Add(FString::Printf(TEXT("externalAccount.%s.id"), *ExternalAccount));
        Keys.Add(FString::Printf(TEXT("externalAccount.%s.displayName"), *ExternalAccount));
        Keys.Add(FString::Printf(TEXT("externalAccount.%s.lastLoginTime.unixTimestampUtc"), *ExternalAccount));
        if (ExternalAccount == TEXT("epic"))
        {
            Keys.Add(FString::Printf(TEXT("externalAccount.%s.country"), *ExternalAccount));
            Keys.Add(FString::Printf(TEXT("externalAccount.%s.nickname"), *ExternalAccount));
            Keys.Add(FString::Printf(TEXT("externalAccount.%s.preferredLanguage"), *ExternalAccount));
        }
    }
    for (const auto &Key : Keys)
    {
        FString Value;
        if (TargetFriend->GetUserAttribute(Key, Value))
        {
            State.Attributes.Add(Key, Value);
        }
    }

    // Return the friend's state.
    return State;
}

void UMOS_GameInstanceSubsystem::ExecuteFriendsSetFriendAlias(
    const FUniqueNetIdRepl &TargetUserId,
    const FString &Alias,
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
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the friends interface, if the online subsystem supports it.
    auto Friends = OSS->GetFriendsInterface();
    if (!Friends.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support friends."));
        return;
    }

    // Set the friend's alias.
    Friends->SetFriendAlias(
        this->LocalUserNum,
        *TargetUserId,
        TEXT(""),
        Alias,
        FOnSetFriendAliasComplete::CreateWeakLambda(
            this,
            [ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(
                 Result)](int32, const FUniqueNetId &, const FString &, const FOnlineError &Error) {
                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return the result.
                ResultWk->OnResult(Error.bSucceeded, Error.ToLogString());
            }));
}

void UMOS_GameInstanceSubsystem::ExecuteFriendsDeleteFriendAlias(
    const FUniqueNetIdRepl &TargetUserId,
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
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the friends interface, if the online subsystem supports it.
    auto Friends = OSS->GetFriendsInterface();
    if (!Friends.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support friends."));
        return;
    }

    // Delete the friend's alias.
    Friends->DeleteFriendAlias(
        this->LocalUserNum,
        *TargetUserId,
        TEXT(""),
        FOnDeleteFriendAliasComplete::CreateWeakLambda(
            this,
            [ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(
                 Result)](int32, const FUniqueNetId &, const FString &, const FOnlineError &Error) {
                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return the result.
                ResultWk->OnResult(Error.bSucceeded, Error.ToLogString());
            }));
}

void UMOS_GameInstanceSubsystem::ExecuteFriendsSendInvite(
    const FUniqueNetIdRepl &TargetUserId,
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
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the friends interface, if the online subsystem supports it.
    auto Friends = OSS->GetFriendsInterface();
    if (!Friends.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support friends."));
        return;
    }

    // Send the friend invite.
    if (!Friends->SendInvite(
            this->LocalUserNum,
            *TargetUserId,
            TEXT(""),
            FOnSendInviteComplete::CreateWeakLambda(
                this,
                [ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result)](
                    int32,
                    bool bWasSuccessful,
                    const FUniqueNetId &,
                    const FString &,
                    const FString &ErrorStr) {
                    // Make sure the result callback is still valid.
                    if (!ResultWk.IsValid())
                    {
                        return;
                    }

                    // Return the result.
                    ResultWk->OnResult(bWasSuccessful, ErrorStr);
                })))
    {
        // Call failed to start.
        Result->OnResult(false, TEXT("SendInvite call failed to start."));
    }
}

void UMOS_GameInstanceSubsystem::ExecuteFriendsAcceptInvite(
    const FUniqueNetIdRepl &TargetUserId,
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
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the friends interface, if the online subsystem supports it.
    auto Friends = OSS->GetFriendsInterface();
    if (!Friends.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support friends."));
        return;
    }

    // Accept the friend invite.
    if (!Friends->AcceptInvite(
            this->LocalUserNum,
            *TargetUserId,
            TEXT(""),
            FOnAcceptInviteComplete::CreateWeakLambda(
                this,
                [ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result)](
                    int32,
                    bool bWasSuccessful,
                    const FUniqueNetId &,
                    const FString &,
                    const FString &ErrorStr) {
                    // Make sure the result callback is still valid.
                    if (!ResultWk.IsValid())
                    {
                        return;
                    }

                    // Return the result.
                    ResultWk->OnResult(bWasSuccessful, ErrorStr);
                })))
    {
        // Call failed to start.
        Result->OnResult(false, TEXT("AcceptInvite call failed to start."));
    }
}

void UMOS_GameInstanceSubsystem::ExecuteFriendsRejectInvite(
    const FUniqueNetIdRepl &TargetUserId,
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
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the friends interface, if the online subsystem supports it.
    auto Friends = OSS->GetFriendsInterface();
    if (!Friends.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support friends."));
        return;
    }

    // Register an event so we can receive the reject outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = Friends->AddOnRejectInviteCompleteDelegate_Handle(
        this->LocalUserNum,
        FOnRejectInviteCompleteDelegate::CreateWeakLambda(
            this,
            [this, Friends, CallbackHandle, TargetUserId, ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result)](
                int32 CallbackLocalUserNum,
                bool bCallbackWasSuccessful,
                const FUniqueNetId &CallbackFriendId,
                const FString &,
                const FString &CallbackErrorStr) {
                // Check if this callback is for us.
                if (CallbackLocalUserNum != this->LocalUserNum || CallbackFriendId != *TargetUserId)
                {
                    // This callback isn't for our call.
                    return;
                }

                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return whether the reject invite was successful.
                ResultWk->OnResult(bCallbackWasSuccessful, CallbackErrorStr);

                // Unregister this callback since we've handled the call we care about.
                Friends->ClearOnRejectInviteCompleteDelegate_Handle(this->LocalUserNum, *CallbackHandle);
            }));

    // Reject the friend invite.
    if (!Friends->RejectInvite(this->LocalUserNum, *TargetUserId, TEXT("")))
    {
        // The call failed to start; unregister callback handler.
        Friends->ClearOnRejectInviteCompleteDelegate_Handle(this->LocalUserNum, *CallbackHandle);
        Result->OnResult(false, TEXT("RejectInvite call failed to start."));
    }
}

void UMOS_GameInstanceSubsystem::ExecuteFriendsDeleteFriend(
    const FUniqueNetIdRepl &TargetUserId,
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
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the friends interface, if the online subsystem supports it.
    auto Friends = OSS->GetFriendsInterface();
    if (!Friends.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support friends."));
        return;
    }

    // Register an event so we can receive the delete outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = Friends->AddOnDeleteFriendCompleteDelegate_Handle(
        this->LocalUserNum,
        FOnDeleteFriendCompleteDelegate::CreateWeakLambda(
            this,
            [this, Friends, CallbackHandle, TargetUserId, ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result)](
                int32 CallbackLocalUserNum,
                bool bCallbackWasSuccessful,
                const FUniqueNetId &CallbackFriendId,
                const FString &,
                const FString &CallbackErrorStr) {
                // Check if this callback is for us.
                if (CallbackLocalUserNum != this->LocalUserNum || CallbackFriendId != *TargetUserId)
                {
                    // This callback isn't for our call.
                    return;
                }

                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return whether the deletion was successful.
                ResultWk->OnResult(bCallbackWasSuccessful, CallbackErrorStr);

                // Unregister this callback since we've handled the call we care about.
                Friends->ClearOnDeleteFriendCompleteDelegate_Handle(this->LocalUserNum, *CallbackHandle);
            }));

    // Delete the friend.
    if (!Friends->DeleteFriend(this->LocalUserNum, *TargetUserId, TEXT("")))
    {
        // The call failed to start; unregister callback handler.
        Friends->ClearOnDeleteFriendCompleteDelegate_Handle(this->LocalUserNum, *CallbackHandle);
        Result->OnResult(false, TEXT("DeleteFriend call failed to start."));
    }
}

void UMOS_GameInstanceSubsystem::ExecuteFriendsQueryRecentPlayers(UMOS_AsyncResult *Result)
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
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the friends interface, if the online subsystem supports it.
    auto Friends = OSS->GetFriendsInterface();
    if (!Friends.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support friends."));
        return;
    }

    // Register an event so we can receive the query outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle =
        Friends->AddOnQueryRecentPlayersCompleteDelegate_Handle(FOnQueryRecentPlayersCompleteDelegate::CreateWeakLambda(
            this,
            [Friends, CallbackHandle, UserId, ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result)](
                const FUniqueNetId &CallbackUserId,
                const FString &,
                bool bCallbackWasSuccessful,
                const FString &CallbackError) {
                // Check if this callback is for us.
                if (CallbackUserId != *UserId)
                {
                    // This callback isn't for our call.
                    return;
                }

                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return whether the query was successful.
                ResultWk->OnResult(bCallbackWasSuccessful, CallbackError);

                // Unregister this callback since we've handled the call we care about.
                Friends->ClearOnQueryRecentPlayersCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Ask the online subsystem to query the recent players. We don't return recent players inside
    // ExecuteFriendsQueryRecentPlayers; we're just getting the online subsystem to cache them so that a later
    // GetFriendsCurrentRecentPlayers can return them.
    if (!Friends->QueryRecentPlayers(*UserId, TEXT("")))
    {
        // The call failed to start; unregister callback handler.
        Friends->ClearOnQueryRecentPlayersCompleteDelegate_Handle(*CallbackHandle);
        Result->OnResult(false, TEXT("QueryRecentPlayers call failed to start."));
    }
}

TArray<FMOSFriendsRecentPlayerState> UMOS_GameInstanceSubsystem::GetFriendsCurrentRecentPlayers() const
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        return TArray<FMOSFriendsRecentPlayerState>();
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the friends interface, if the online subsystem supports it.
    auto Friends = OSS->GetFriendsInterface();
    if (!Friends.IsValid())
    {
        return TArray<FMOSFriendsRecentPlayerState>();
    }

    // Get the recent players.
    TArray<FMOSFriendsRecentPlayerState> Results;
    TArray<TSharedRef<FOnlineRecentPlayer>> RecentPlayers;
    if (Friends->GetRecentPlayers(*UserId, TEXT(""), RecentPlayers))
    {
        for (const auto &RecentPlayer : RecentPlayers)
        {
            FMOSFriendsRecentPlayerState State;
            State.Id = RecentPlayer->GetUserId();
            State.DisplayName = RecentPlayer->GetDisplayName();
            Results.Add(State);
        }
    }
    return Results;
}

void UMOS_GameInstanceSubsystem::ExecuteFriendsBlockPlayer(
    const FUniqueNetIdRepl &TargetUserId,
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
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the friends interface, if the online subsystem supports it.
    auto Friends = OSS->GetFriendsInterface();
    if (!Friends.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support friends."));
        return;
    }

    // Register an event so we can receive the block outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = Friends->AddOnBlockedPlayerCompleteDelegate_Handle(
        this->LocalUserNum,
        FOnBlockedPlayerCompleteDelegate::CreateWeakLambda(
            this,
            [this, Friends, CallbackHandle, TargetUserId, ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result)](
                int32 CallbackLocalUserNum,
                bool bCallbackWasSuccessful,
                const FUniqueNetId &CallbackTargetUserId,
                const FString &,
                const FString &CallbackErrorStr) {
                // Check if this callback is for us.
                if (CallbackLocalUserNum != this->LocalUserNum || CallbackTargetUserId != *TargetUserId)
                {
                    // This callback isn't for our call.
                    return;
                }

                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return whether the block was successful.
                ResultWk->OnResult(bCallbackWasSuccessful, CallbackErrorStr);

                // Unregister this callback since we've handled the call we care about.
                Friends->ClearOnBlockedPlayerCompleteDelegate_Handle(this->LocalUserNum, *CallbackHandle);
            }));

    // Block the target player.
    if (!Friends->BlockPlayer(this->LocalUserNum, *TargetUserId))
    {
        // The call failed to start; unregister callback handler.
        Friends->ClearOnBlockedPlayerCompleteDelegate_Handle(this->LocalUserNum, *CallbackHandle);
        Result->OnResult(false, TEXT("BlockPlayer call failed to start."));
    }
}

void UMOS_GameInstanceSubsystem::ExecuteFriendsUnblockPlayer(
    const FUniqueNetIdRepl &TargetUserId,
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
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the friends interface, if the online subsystem supports it.
    auto Friends = OSS->GetFriendsInterface();
    if (!Friends.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support friends."));
        return;
    }

    // Register an event so we can receive the unblock outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = Friends->AddOnUnblockedPlayerCompleteDelegate_Handle(
        this->LocalUserNum,
        FOnUnblockedPlayerCompleteDelegate::CreateWeakLambda(
            this,
            [this, Friends, CallbackHandle, TargetUserId, ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result)](
                int32 CallbackLocalUserNum,
                bool bCallbackWasSuccessful,
                const FUniqueNetId &CallbackTargetUserId,
                const FString &,
                const FString &CallbackErrorStr) {
                // Check if this callback is for us.
                if (CallbackLocalUserNum != this->LocalUserNum || CallbackTargetUserId != *TargetUserId)
                {
                    // This callback isn't for our call.
                    return;
                }

                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return whether the unblock was successful.
                ResultWk->OnResult(bCallbackWasSuccessful, CallbackErrorStr);

                // Unregister this callback since we've handled the call we care about.
                Friends->ClearOnUnblockedPlayerCompleteDelegate_Handle(this->LocalUserNum, *CallbackHandle);
            }));

    // Unblock the target player.
    if (!Friends->UnblockPlayer(this->LocalUserNum, *TargetUserId))
    {
        // The call failed to start; unregister callback handler.
        Friends->ClearOnUnblockedPlayerCompleteDelegate_Handle(this->LocalUserNum, *CallbackHandle);
        Result->OnResult(false, TEXT("UnblockPlayer call failed to start."));
    }
}

void UMOS_GameInstanceSubsystem::ExecuteFriendsQueryBlockedPlayers(UMOS_AsyncResult *Result)
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
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the friends interface, if the online subsystem supports it.
    auto Friends = OSS->GetFriendsInterface();
    if (!Friends.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support friends."));
        return;
    }

    // Register an event so we can receive the query outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = Friends->AddOnQueryBlockedPlayersCompleteDelegate_Handle(
        FOnQueryBlockedPlayersCompleteDelegate::CreateWeakLambda(
            this,
            [Friends, CallbackHandle, UserId, ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result)](
                const FUniqueNetId &CallbackUserId,
                bool bCallbackWasSuccessful,
                const FString &CallbackError) {
                // Check if this callback is for us.
                if (CallbackUserId != *UserId)
                {
                    // This callback isn't for our call.
                    return;
                }

                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return whether the query was successful.
                ResultWk->OnResult(bCallbackWasSuccessful, CallbackError);

                // Unregister this callback since we've handled the call we care about.
                Friends->ClearOnQueryBlockedPlayersCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Ask the online subsystem to query the blocked players. We don't return blocked players inside
    // ExecuteFriendsQueryBlockedPlayers; we're just getting the online subsystem to cache them so that a later
    // GetFriendsCurrentBlockedPlayers can return them.
    if (!Friends->QueryBlockedPlayers(*UserId))
    {
        // The call failed to start; unregister callback handler.
        Friends->ClearOnQueryBlockedPlayersCompleteDelegate_Handle(*CallbackHandle);
        Result->OnResult(false, TEXT("QueryBlockedPlayers call failed to start."));
    }
}

TArray<FMOSFriendsBlockedPlayerState> UMOS_GameInstanceSubsystem::GetFriendsCurrentBlockedPlayers() const
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        return TArray<FMOSFriendsBlockedPlayerState>();
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the friends interface, if the online subsystem supports it.
    auto Friends = OSS->GetFriendsInterface();
    if (!Friends.IsValid())
    {
        return TArray<FMOSFriendsBlockedPlayerState>();
    }

    // Get the blocked players.
    TArray<FMOSFriendsBlockedPlayerState> Results;
    TArray<TSharedRef<FOnlineBlockedPlayer>> BlockedPlayers;
    if (Friends->GetBlockedPlayers(*UserId, BlockedPlayers))
    {
        for (const auto &BlockedPlayer : BlockedPlayers)
        {
            FMOSFriendsBlockedPlayerState State;
            State.Id = BlockedPlayer->GetUserId();
            State.DisplayName = BlockedPlayer->GetDisplayName();
            Results.Add(State);
        }
    }
    return Results;
}

void UMOS_GameInstanceSubsystem::ExecuteFriendsSendInviteByFriendCode(
    const FString &FriendCode,
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
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the friends interface, if the online subsystem supports it.
    auto Friends = OSS->GetFriendsInterface();
    if (!Friends.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support friends."));
        return;
    }

    // Get the user interface, if the online subsystem supports it.
    auto User = OSS->GetUserInterface();
    if (!User.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support user querying."));
        return;
    }

    // Query user ID mapping by friend code.
    if (!User->QueryUserIdMapping(
            *UserId,
            FString::Printf(TEXT("FriendCode:%s"), *FriendCode),
            IOnlineUser::FOnQueryUserMappingComplete::CreateWeakLambda(
                this,
                [this, ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result)](
                    bool bWasSuccessful,
                    const FUniqueNetId &,
                    const FString &,
                    const FUniqueNetId &FoundUserId,
                    const FString &Error) {
                    // Make sure the result callback is still valid.
                    if (!ResultWk.IsValid())
                    {
                        return;
                    }

                    // If the user couldn't be found, return now.
                    if (!bWasSuccessful)
                    {
                        ResultWk->OnResult(bWasSuccessful, Error);
                        return;
                    }

                    // Forward onto our existing SendInvite handling function, which
                    // will also call Result->OnResult.
                    this->ExecuteFriendsSendInvite(FoundUserId, ResultWk.Get());
                })))
    {
        // The call failed to start.
        Result->OnResult(false, TEXT("QueryUserIdMapping call failed to start."));
    }
}

FString UMOS_GameInstanceSubsystem::GetFriendsCurrentFriendCode() const
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        return TEXT("");
    }

    // Get the identity interface.
    auto Identity = OSS->GetIdentityInterface();
    if (!Identity.IsValid())
    {
        return TEXT("");
    }

    // Get the user ID if the user is signed in.
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    if (!UserId.IsValid())
    {
        return TEXT("");
    }

    // Get the signed in user's account.
    auto UserAccount = Identity->GetUserAccount(*UserId);
    checkf(UserId.IsValid(), TEXT("Expected GetUserAccount to return a valid account."));

    // Return the value of the "friendCode" auth attribute.
    FString Value;
    return UserAccount->GetAuthAttribute(TEXT("friendCode"), Value) ? Value : TEXT("");
}