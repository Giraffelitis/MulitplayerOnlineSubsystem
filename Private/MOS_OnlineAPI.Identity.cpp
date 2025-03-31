// Copyright June Rhodes. MIT Licensed.

#include "MultiplayerOnlineSubsystem/Public/MOS_GameInstanceSubsystem.h"

#include "OnlineSubsystemUtils.h"

FUniqueNetIdRepl UMOS_GameInstanceSubsystem::GetIdentityUniqueNetId(const FString &UniqueNetId)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        return FUniqueNetIdRepl();
    }

    // Get the identity interface.
    auto Identity = OSS->GetIdentityInterface();
    if (!Identity.IsValid())
    {
        return FUniqueNetIdRepl();
    }

    // Return the currently signed in user (or an empty FUniqueNetIdRepl if no-one is signed in).
    return Identity->GetUniquePlayerId(this->LocalUserNum);
}