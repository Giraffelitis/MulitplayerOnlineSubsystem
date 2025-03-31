// Copyright June Rhodes. MIT Licensed.

#include "MultiplayerOnlineSusbsystem/Public/MOS_GameInstanceSubsystem.h"

#include "Interfaces/OnlinePartyInterface.h"
#include "OnlineSubsystemUtils.h"

TArray<FOSSInterfaceListEntry> UMOS_GameInstanceSubsystem::GetPartiesJoinedParties() const
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        return TArray<FOSSInterfaceListEntry>();
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the party interface, if the online subsystem supports it.
    auto PartySystem = OSS->GetPartyInterface();
    if (!PartySystem.IsValid())
    {
        return TArray<FOSSInterfaceListEntry>();
    }

    // Get the list of parties currently joined.
    TArray<FOSSInterfaceListEntry> Entries;
    TArray<TSharedRef<const FOnlinePartyId>> PartyIds;
    PartySystem->GetJoinedParties(*UserId, PartyIds);
    for (const auto &PartyId : PartyIds)
    {
        FOSSInterfaceListEntry Entry;
        Entry.Id = PartyId->ToString();
        Entry.DisplayName = FText::FromString(PartyId->ToString());
        Entries.Add(Entry);
    }
    return Entries;
}

void UMOS_GameInstanceSubsystem::ExecutePartiesCreateParty(UMOS_AsyncResult *Result)
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

    // Get the party interface, if the online subsystem supports it.
    auto PartySystem = OSS->GetPartyInterface();
    if (!PartySystem.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support parties."));
        return;
    }

    // Create the party.
    FPartyConfiguration PartyConfiguration;
    PartyConfiguration.JoinRequestAction = EJoinRequestAction::AutoApprove;
    PartyConfiguration.PresencePermissions = PartySystemPermissions::EPermissionType::Anyone;
    PartyConfiguration.InvitePermissions = PartySystemPermissions::EPermissionType::Anyone;
    PartyConfiguration.bChatEnabled = true;
    PartyConfiguration.bShouldRemoveOnDisconnection = false;
    PartyConfiguration.bIsAcceptingMembers = true;
    PartyConfiguration.NotAcceptingMembersReason = 0;
    PartyConfiguration.MaxMembers = 4;
    PartyConfiguration.Nickname = TEXT("");
    PartyConfiguration.Description = TEXT("");
    PartyConfiguration.Password = TEXT("");
    PartySystem->CreateParty(
        *UserId,
        PartySystem->GetPrimaryPartyTypeId(),
        PartyConfiguration,
        FOnCreatePartyComplete::CreateWeakLambda(
            this,
            [ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result)](
                const FUniqueNetId &,
                const TSharedPtr<const FOnlinePartyId> &PartyId,
                const ECreatePartyCompletionResult PartyResult) {
                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return the success status.
                FString ErrorMessage;
                switch (PartyResult)
                {
                case ECreatePartyCompletionResult::UnknownClientFailure:
                    ErrorMessage = TEXT("UnknownClientFailure");
                    break;
                case ECreatePartyCompletionResult::AlreadyInPartyOfSpecifiedType:
                    ErrorMessage = TEXT("AlreadyInPartyOfSpecifiedType");
                    break;
                case ECreatePartyCompletionResult::AlreadyCreatingParty:
                    ErrorMessage = TEXT("AlreadyCreatingParty");
                    break;
                case ECreatePartyCompletionResult::AlreadyInParty:
                    ErrorMessage = TEXT("AlreadyInParty");
                    break;
                case ECreatePartyCompletionResult::FailedToCreateMucRoom:
                    ErrorMessage = TEXT("FailedToCreateMucRoom");
                    break;
                case ECreatePartyCompletionResult::NoResponse:
                    ErrorMessage = TEXT("NoResponse");
                    break;
                case ECreatePartyCompletionResult::LoggedOut:
                    ErrorMessage = TEXT("LoggedOut");
                    break;
                case ECreatePartyCompletionResult::NotPrimaryUser:
                    ErrorMessage = TEXT("NotPrimaryUser");
                    break;
                case ECreatePartyCompletionResult::UnknownInternalFailure:
                    ErrorMessage = TEXT("UnknownInternalFailure");
                    break;
                default:
                case ECreatePartyCompletionResult::Succeeded:
                    break;
                }
                ResultWk->OnResult(PartyResult == ECreatePartyCompletionResult::Succeeded, ErrorMessage);
            }));
}

TArray<FOSSInterfaceListEntry> UMOS_GameInstanceSubsystem::GetPartiesCurrentInvites() const
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        return TArray<FOSSInterfaceListEntry>();
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the party interface, if the online subsystem supports it.
    auto PartySystem = OSS->GetPartyInterface();
    if (!PartySystem.IsValid())
    {
        return TArray<FOSSInterfaceListEntry>();
    }

    // Get the list of current party invites.
    TArray<FOSSInterfaceListEntry> Entries;
    TArray<IOnlinePartyJoinInfoConstRef> PartyInvites;
    PartySystem->GetPendingInvites(*UserId, PartyInvites);
    for (const auto &PartyInvite : PartyInvites)
    {
        FOSSInterfaceListEntry Entry;
        Entry.Id = PartyInvite->GetPartyId()->ToString();
        Entry.DisplayName = FText::Format(
            NSLOCTEXT("Party", "PartyInviteDisplayName", "Party: {0}\nInvited By UID: {1}\nInvited By Name: {2}"),
            FText::FromString(PartyInvite->GetPartyId()->ToString()),
            FText::FromString(PartyInvite->GetSourceUserId()->ToString()),
            FText::FromString(PartyInvite->GetSourceDisplayName()));
        Entries.Add(Entry);
    }
    return Entries;
}

void UMOS_GameInstanceSubsystem::ExecutePartiesAcceptInvite(
    const FOSSInterfaceListEntry &Invite,
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

    // Get the party interface, if the online subsystem supports it.
    auto PartySystem = OSS->GetPartyInterface();
    if (!PartySystem.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support parties."));
        return;
    }

    // Locate the invite so we can join it.
    IOnlinePartyJoinInfoConstPtr SelectedPartyInvite;
    TArray<IOnlinePartyJoinInfoConstRef> PartyInvites;
    PartySystem->GetPendingInvites(*UserId, PartyInvites);
    for (const auto &PartyInvite : PartyInvites)
    {
        if (PartyInvite->GetPartyId()->ToString() == Invite.Id)
        {
            SelectedPartyInvite = PartyInvite;
            break;
        }
    }
    if (!SelectedPartyInvite.IsValid())
    {
        Result->OnResult(false, TEXT("Invite not found."));
        return;
    }

    // Join the party.
    PartySystem->JoinParty(
        *UserId,
        *SelectedPartyInvite,
        FOnJoinPartyComplete::CreateWeakLambda(
            this,
            [ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result)](
                const FUniqueNetId &,
                const FOnlinePartyId &,
                const EJoinPartyCompletionResult PartyResult,
                const int32) {
                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return the success status.
                FString ErrorMessage;
                switch (PartyResult)
                {
                case EJoinPartyCompletionResult::UnknownClientFailure:
                    ErrorMessage = TEXT("UnknownClientFailure");
                    break;
                case EJoinPartyCompletionResult::BadBuild:
                    ErrorMessage = TEXT("BadBuild");
                    break;
                case EJoinPartyCompletionResult::InvalidAccessKey:
                    ErrorMessage = TEXT("InvalidAccessKey");
                    break;
                case EJoinPartyCompletionResult::AlreadyInLeadersJoiningList:
                    ErrorMessage = TEXT("AlreadyInLeadersJoiningList");
                    break;
                case EJoinPartyCompletionResult::AlreadyInLeadersPartyRoster:
                    ErrorMessage = TEXT("AlreadyInLeadersPartyRoster");
                    break;
                case EJoinPartyCompletionResult::NoSpace:
                    ErrorMessage = TEXT("NoSpace");
                    break;
                case EJoinPartyCompletionResult::NotApproved:
                    ErrorMessage = TEXT("NotApproved");
                    break;
                case EJoinPartyCompletionResult::RequesteeNotMember:
                    ErrorMessage = TEXT("RequesteeNotMember");
                    break;
                case EJoinPartyCompletionResult::RequesteeNotLeader:
                    ErrorMessage = TEXT("RequesteeNotLeader");
                    break;
                case EJoinPartyCompletionResult::NoResponse:
                    ErrorMessage = TEXT("NoResponse");
                    break;
                case EJoinPartyCompletionResult::LoggedOut:
                    ErrorMessage = TEXT("LoggedOut");
                    break;
#if !REDPOINT_EXAMPLE_UE_5_5_OR_LATER
                case EJoinPartyCompletionResult::UnableToRejoin:
                    ErrorMessage = TEXT("UnableToRejoin");
                    break;
#endif
                case EJoinPartyCompletionResult::IncompatiblePlatform:
                    ErrorMessage = TEXT("IncompatiblePlatform");
                    break;
                case EJoinPartyCompletionResult::AlreadyJoiningParty:
                    ErrorMessage = TEXT("AlreadyJoiningParty");
                    break;
                case EJoinPartyCompletionResult::AlreadyInParty:
                    ErrorMessage = TEXT("AlreadyInParty");
                    break;
                case EJoinPartyCompletionResult::JoinInfoInvalid:
                    ErrorMessage = TEXT("JoinInfoInvalid");
                    break;
                case EJoinPartyCompletionResult::AlreadyInPartyOfSpecifiedType:
                    ErrorMessage = TEXT("AlreadyInPartyOfSpecifiedType");
                    break;
                case EJoinPartyCompletionResult::MessagingFailure:
                    ErrorMessage = TEXT("MessagingFailure");
                    break;
                case EJoinPartyCompletionResult::GameSpecificReason:
                    ErrorMessage = TEXT("GameSpecificReason");
                    break;
                case EJoinPartyCompletionResult::MismatchedApp:
                    ErrorMessage = TEXT("MismatchedApp");
                    break;
                case EJoinPartyCompletionResult::UnknownInternalFailure:
                    ErrorMessage = TEXT("UnknownInternalFailure");
                    break;
                default:
                case EJoinPartyCompletionResult::Succeeded:
                    break;
                }
                ResultWk->OnResult(PartyResult == EJoinPartyCompletionResult::Succeeded, ErrorMessage);
            }));
}

TArray<FUniqueNetIdRepl> UMOS_GameInstanceSubsystem::GetPartiesCurrentMembers(const FString &PartyIdStr) const
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

    // Get the party interface, if the online subsystem supports it.
    auto PartySystem = OSS->GetPartyInterface();
    if (!PartySystem.IsValid())
    {
        return TArray<FUniqueNetIdRepl>();
    }

    // Get the requested party.
    TSharedPtr<const FOnlinePartyId> SelectedPartyId;
    TArray<TSharedRef<const FOnlinePartyId>> PartyIds;
    PartySystem->GetJoinedParties(*UserId, PartyIds);
    for (const auto &PartyId : PartyIds)
    {
        if (PartyId->ToString() == PartyIdStr)
        {
            SelectedPartyId = PartyId;
            break;
        }
    }
    if (!SelectedPartyId.IsValid())
    {
        return TArray<FUniqueNetIdRepl>();
    }

    // Get the list of members in that party.
    TArray<FUniqueNetIdRepl> Entries;
    TArray<FOnlinePartyMemberConstRef> PartyMembers;
    PartySystem->GetPartyMembers(*UserId, *SelectedPartyId, PartyMembers);
    for (const auto &PartyMember : PartyMembers)
    {
        Entries.Add(PartyMember->GetUserId());
    }
    return Entries;
}

void UMOS_GameInstanceSubsystem::ExecutePartiesLeaveParty(const FString &PartyIdStr, UMOS_AsyncResult *Result)
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

    // Get the party interface, if the online subsystem supports it.
    auto PartySystem = OSS->GetPartyInterface();
    if (!PartySystem.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support parties."));
        return;
    }

    // Locate the party so we can leave it.
    TSharedPtr<const FOnlinePartyId> SelectedPartyId;
    TArray<TSharedRef<const FOnlinePartyId>> PartyIds;
    PartySystem->GetJoinedParties(*UserId, PartyIds);
    for (const auto &PartyId : PartyIds)
    {
        if (PartyId->ToString() == PartyIdStr)
        {
            SelectedPartyId = PartyId;
            break;
        }
    }
    if (!SelectedPartyId.IsValid())
    {
        Result->OnResult(false, TEXT("Party not found."));
        return;
    }

    // Leave the party.
    PartySystem->LeaveParty(
        *UserId,
        *SelectedPartyId,
        true,
        FOnLeavePartyComplete::CreateWeakLambda(
            this,
            [ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(
                 Result)](const FUniqueNetId &, const FOnlinePartyId &, const ELeavePartyCompletionResult PartyResult) {
                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return the success status.
                FString ErrorMessage;
                switch (PartyResult)
                {
                case ELeavePartyCompletionResult::UnknownClientFailure:
                    ErrorMessage = TEXT("UnknownClientFailure");
                    break;
                case ELeavePartyCompletionResult::NoResponse:
                    ErrorMessage = TEXT("NoResponse");
                    break;
                case ELeavePartyCompletionResult::LoggedOut:
                    ErrorMessage = TEXT("LoggedOut");
                    break;
                case ELeavePartyCompletionResult::UnknownParty:
                    ErrorMessage = TEXT("UnknownParty");
                    break;
                case ELeavePartyCompletionResult::LeavePending:
                    ErrorMessage = TEXT("LeavePending");
                    break;
                case ELeavePartyCompletionResult::UnknownLocalUser:
                    ErrorMessage = TEXT("UnknownLocalUser");
                    break;
                case ELeavePartyCompletionResult::NotMember:
                    ErrorMessage = TEXT("NotMember");
                    break;
                case ELeavePartyCompletionResult::MessagingFailure:
                    ErrorMessage = TEXT("MessagingFailure");
                    break;
                case ELeavePartyCompletionResult::UnknownTransportFailure:
                    ErrorMessage = TEXT("UnknownTransportFailure");
                    break;
                case ELeavePartyCompletionResult::UnknownInternalFailure:
                    ErrorMessage = TEXT("UnknownInternalFailure");
                    break;
                default:
                case ELeavePartyCompletionResult::Succeeded:
                    break;
                }
                ResultWk->OnResult(PartyResult == ELeavePartyCompletionResult::Succeeded, ErrorMessage);
            }));
}

void UMOS_GameInstanceSubsystem::ExecutePartiesKickMember(
    const FString &PartyIdStr,
    const FUniqueNetIdRepl &MemberId,
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

    // Get the party interface, if the online subsystem supports it.
    auto PartySystem = OSS->GetPartyInterface();
    if (!PartySystem.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support parties."));
        return;
    }

    // Locate the party so we can leave it.
    TSharedPtr<const FOnlinePartyId> SelectedPartyId;
    TArray<TSharedRef<const FOnlinePartyId>> PartyIds;
    PartySystem->GetJoinedParties(*UserId, PartyIds);
    for (const auto &PartyId : PartyIds)
    {
        if (PartyId->ToString() == PartyIdStr)
        {
            SelectedPartyId = PartyId;
            break;
        }
    }
    if (!SelectedPartyId.IsValid())
    {
        Result->OnResult(false, TEXT("Party not found."));
        return;
    }

    // Kick the member from the party.
    PartySystem->KickMember(
        *UserId,
        *SelectedPartyId,
        *MemberId,
        FOnKickPartyMemberComplete::CreateWeakLambda(
            this,
            [ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result)](
                const FUniqueNetId &,
                const FOnlinePartyId &,
                const FUniqueNetId &,
                const EKickMemberCompletionResult PartyResult) {
                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return the success status.
                FString ErrorMessage;
                switch (PartyResult)
                {
                case EKickMemberCompletionResult::UnknownClientFailure:
                    ErrorMessage = TEXT("UnknownClientFailure");
                    break;
                case EKickMemberCompletionResult::UnknownParty:
                    ErrorMessage = TEXT("UnknownParty");
                    break;
                case EKickMemberCompletionResult::LocalMemberNotMember:
                    ErrorMessage = TEXT("LocalMemberNotMember");
                    break;
                case EKickMemberCompletionResult::LocalMemberNotLeader:
                    ErrorMessage = TEXT("LocalMemberNotLeader");
                    break;
                case EKickMemberCompletionResult::RemoteMemberNotMember:
                    ErrorMessage = TEXT("RemoteMemberNotMember");
                    break;
                case EKickMemberCompletionResult::MessagingFailure:
                    ErrorMessage = TEXT("MessagingFailure");
                    break;
                case EKickMemberCompletionResult::NoResponse:
                    ErrorMessage = TEXT("NoResponse");
                    break;
                case EKickMemberCompletionResult::LoggedOut:
                    ErrorMessage = TEXT("LoggedOut");
                    break;
                case EKickMemberCompletionResult::UnknownInternalFailure:
                    ErrorMessage = TEXT("UnknownInternalFailure");
                    break;
                default:
                case EKickMemberCompletionResult::Succeeded:
                    break;
                }
                ResultWk->OnResult(PartyResult == EKickMemberCompletionResult::Succeeded, ErrorMessage);
            }));
}

void UMOS_GameInstanceSubsystem::ExecutePartiesInviteFriend(
    const FString &PartyIdStr,
    const FUniqueNetIdRepl &MemberId,
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

    // Get the party interface, if the online subsystem supports it.
    auto PartySystem = OSS->GetPartyInterface();
    if (!PartySystem.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support parties."));
        return;
    }

    // Locate the party so we can send the invitation.
    TSharedPtr<const FOnlinePartyId> SelectedPartyId;
    TArray<TSharedRef<const FOnlinePartyId>> PartyIds;
    PartySystem->GetJoinedParties(*UserId, PartyIds);
    for (const auto &PartyId : PartyIds)
    {
        if (PartyId->ToString() == PartyIdStr)
        {
            SelectedPartyId = PartyId;
            break;
        }
    }
    if (!SelectedPartyId.IsValid())
    {
        Result->OnResult(false, TEXT("Party not found."));
        return;
    }

    // Send the invitation to the friend.
    PartySystem->SendInvitation(
        *UserId,
        *SelectedPartyId,
        FPartyInvitationRecipient(*MemberId),
        FOnSendPartyInvitationComplete::CreateWeakLambda(
            this,
            [ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result)](
                const FUniqueNetId &,
                const FOnlinePartyId &,
                const FUniqueNetId &,
                const ESendPartyInvitationCompletionResult PartyResult) {
                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return the success status.
                FString ErrorMessage;
                switch (PartyResult)
                {
                case ESendPartyInvitationCompletionResult::NotLoggedIn:
                    ErrorMessage = TEXT("NotLoggedIn");
                    break;
                case ESendPartyInvitationCompletionResult::InvitePending:
                    ErrorMessage = TEXT("InvitePending");
                    break;
                case ESendPartyInvitationCompletionResult::AlreadyInParty:
                    ErrorMessage = TEXT("AlreadyInParty");
                    break;
                case ESendPartyInvitationCompletionResult::PartyFull:
                    ErrorMessage = TEXT("PartyFull");
                    break;
                case ESendPartyInvitationCompletionResult::NoPermission:
                    ErrorMessage = TEXT("NoPermission");
                    break;
                case ESendPartyInvitationCompletionResult::RateLimited:
                    ErrorMessage = TEXT("RateLimited");
                    break;
                case ESendPartyInvitationCompletionResult::UnknownInternalFailure:
                    ErrorMessage = TEXT("UnknownInternalFailure");
                    break;
                default:
                case ESendPartyInvitationCompletionResult::Succeeded:
                    break;
                }
                ResultWk->OnResult(PartyResult == ESendPartyInvitationCompletionResult::Succeeded, ErrorMessage);
            }));
}