// Copyright June Rhodes. MIT Licensed.

#include "MultiplayerOnlineSubsystem/Public/MOS_GameInstanceSubsystem.h"

#include "Interfaces/OnlineStatsInterface.h"
#include "OnlineSubsystemUtils.h"


void UMOS_GameInstanceSubsystem::ExecuteStatsQueryStats(UMOS_QueryStatsAsyncResult *Result)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        Result->OnResult(false, TArray<FMOSStatsStatState>(), TEXT("Online subsystem is not available."));
        return;
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the stats interface, if the online subsystem supports it.
    auto Stats = OSS->GetStatsInterface();
    if (!Stats.IsValid())
    {
        Result->OnResult(false, TArray<FMOSStatsStatState>(), TEXT("Online subsystem does not support stats."));
        return;
    }

    // Set the stat names we want to query.
    TArray<FString> StatNames;
    StatNames.Add(TEXT("TestLatest"));
    StatNames.Add(TEXT("TestScore"));

    // Query the stats.
    Stats->QueryStats(
        UserId.ToSharedRef(),
        TArray<FUniqueNetIdRef>{UserId.ToSharedRef()},
        StatNames,
        FOnlineStatsQueryUsersStatsComplete::CreateWeakLambda(
            this,
            [UserId, ResultWk = TSoftObjectPtr<UMOS_QueryStatsAsyncResult>(Result)](
                const FOnlineError &CallbackResult,
                const TArray<TSharedRef<const FOnlineStatsUserStats>> &CallbackUsersStats) {
                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return if the call failed.
                if (!CallbackResult.bSucceeded || CallbackUsersStats.Num() == 0)
                {
                    ResultWk->OnResult(false, TArray<FMOSStatsStatState>(), TEXT("QueryStats call failed."));
                    return;
                }

                // Convert the results.
                TArray<FMOSStatsStatState> Entries;
                for (const auto &StatKV : CallbackUsersStats[0]->Stats)
                {
                    FMOSStatsStatState Entry;
                    Entry.Name = StatKV.Key;
                    int32 Value;
                    StatKV.Value.GetValue(Value);
                    Entry.CurrentValue = Value;
                    Entries.Add(Entry);
                }
                ResultWk->OnResult(true, Entries, TEXT(""));
            }));
}

void UMOS_GameInstanceSubsystem::ExecuteStatsIngestStat(
    const FString &StatName,
    double StatValue,
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

    // Get the stats interface, if the online subsystem supports it.
    auto Stats = OSS->GetStatsInterface();
    if (!Stats.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support stats."));
        return;
    }

    // Create the stat update map.
    TArray<FOnlineStatsUserUpdatedStats> NewStats;
    NewStats.Add(FOnlineStatsUserUpdatedStats(
        UserId.ToSharedRef(),
        TMap<FString, FOnlineStatUpdate>{
            {StatName, FOnlineStatUpdate((int32)StatValue, FOnlineStatUpdate::EOnlineStatModificationType::Set)}}));

    // Update the stat.
    Stats->UpdateStats(
        UserId.ToSharedRef(),
        NewStats,
        FOnlineStatsUpdateStatsComplete::CreateWeakLambda(
            this,
            [ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result)](const FOnlineError &ResultState) {
                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return the result.
                ResultWk->OnResult(ResultState.bSucceeded, ResultState.ToLogString());
            }));
}