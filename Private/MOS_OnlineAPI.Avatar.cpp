// Copyright June Rhodes. MIT Licensed.

#include "MultiplayerOnlineSubsystem/Public/MOS_GameInstanceSubsystem.h"

#include "MultiplayerOnlineSubsystem/Public/Interfaces/OnlineAvatarInterface.h"

#include "OnlineSubsystemUtils.h"

void UMOS_GameInstanceSubsystem::ExecuteAvatarGetAvatar(
    const FUniqueNetIdRepl &TargetUserId,
    UMOS_GetAvatarAsyncResult *Result)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        Result->OnResult(false, nullptr, TEXT("Online subsystem is not available."));
        return;
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    if (!UserId.IsValid())
    {
        Result->OnResult(false, nullptr, TEXT("The local user is not signed in."));
        return;
    }

    // Get the avatar interface.
    auto Avatar = Online::GetAvatarInterface(OSS);
    if (!Avatar.IsValid())
    {
        Result->OnResult(false, nullptr, TEXT("Online subsystem does not support avatars."));
        return;
    }

    // Query for the target avatar.
    Avatar->GetAvatar(
        *UserId,
        *TargetUserId,
        nullptr,
        FOnGetAvatarComplete::CreateWeakLambda(
            this,
            [ResultWk = TSoftObjectPtr<UMOS_GetAvatarAsyncResult>(
                 Result)](bool bWasSuccessful, TSoftObjectPtr<UTexture> ResultTexture) {
                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return the result.
                ResultWk->OnResult(
                    bWasSuccessful,
                    ResultTexture,
                    bWasSuccessful ? TEXT("") : TEXT("The GetAvatar call failed."));
            }));
}