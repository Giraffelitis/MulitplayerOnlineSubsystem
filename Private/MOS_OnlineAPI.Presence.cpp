// Copyright June Rhodes. MIT Licensed.

#include "MultiplayerOnlineSubsystem/Public/MOS_GameInstanceSubsystem.h"

#include "Interfaces/OnlinePresenceInterface.h"
#include "OnlineSubsystemUtils.h"


TArray<FText> UMOS_GameInstanceSubsystem::GetPresenceAvailableStatuses() const
{
    TArray<FText> Text;
    Text.Add(NSLOCTEXT("PresenceLocalisationText", "PresenceExample1", "Example presence status 1"));
    Text.Add(NSLOCTEXT("PresenceLocalisationText", "PresenceExample2", "Example presence with numeric value {Number}"));
    return Text;
}

void UMOS_GameInstanceSubsystem::ExecutePresenceSetPresence(
    const FText &NewStatus,
    const FString &NewStatusTextNamespace,
    const FString &NewStatusTextKey,
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

    // Get the presence interface, if the online subsystem supports it.
    auto Presence = OSS->GetPresenceInterface();
    if (!Presence.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support presence."));
        return;
    }

    // Update the local user's presence.
    FOnlineUserPresenceStatus PresenceStatus;
    PresenceStatus.State = EOnlinePresenceState::Online;
    PresenceStatus.StatusStr = FString::Printf(TEXT("%s_%s"), *NewStatusTextNamespace, *NewStatusTextKey);
    PresenceStatus.Properties.Add(TEXT("Number"), 200);
    Presence->SetPresence(
        *UserId,
        PresenceStatus,
        IOnlinePresence::FOnPresenceTaskCompleteDelegate::CreateWeakLambda(
            this,
            [ResultWk =
                 TSoftObjectPtr<UMOS_AsyncResult>(Result)](const class FUniqueNetId &, const bool bWasSuccessful) {
                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return the result.
                ResultWk->OnResult(bWasSuccessful, bWasSuccessful ? TEXT("") : TEXT("Failed to set presence"));
            }));
}