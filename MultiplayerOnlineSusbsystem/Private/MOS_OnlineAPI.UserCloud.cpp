// Copyright June Rhodes. MIT Licensed.

#include "MultiplayerOnlineSusbsystem/Public/MOS_GameInstanceSubsystem.h"

#include "MultiplayerOnlineSusbsystem/Public/MOS_SaveGame.h"
#include "Interfaces/OnlineUserCloudInterface.h"
#include "Kismet/GameplayStatics.h"
#include "OnlineSubsystemUtils.h"


void UMOS_GameInstanceSubsystem::ExecuteUserCloudQueryFiles(UMOS_ListAsyncResult *Result)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        Result->OnResult(false, TArray<FOSSInterfaceListEntry>(), TEXT("Online subsystem is not available."));
        return;
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the user cloud interface, if the online subsystem supports it.
    auto UserCloud = OSS->GetUserCloudInterface();
    if (!UserCloud.IsValid())
    {
        Result->OnResult(
            false,
            TArray<FOSSInterfaceListEntry>(),
            TEXT("Online subsystem does not support user cloud."));
        return;
    }

    // Register an event so we can receive the outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = UserCloud->AddOnEnumerateUserFilesCompleteDelegate_Handle(
        FOnEnumerateUserFilesCompleteDelegate::CreateWeakLambda(
            this,
            [UserCloud, CallbackHandle, ResultWk = TSoftObjectPtr<UMOS_ListAsyncResult>(Result), UserId](
                bool bCallbackWasSuccessful,
                const FUniqueNetId &CallbackUserId) {
                // Check if this callback is for us.
                if (*UserId != CallbackUserId)
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
                if (!bCallbackWasSuccessful)
                {
                    ResultWk->OnResult(
                        false,
                        TArray<FOSSInterfaceListEntry>(),
                        TEXT("User cloud enumeration failed."));
                    UserCloud->ClearOnEnumerateUserFilesCompleteDelegate_Handle(*CallbackHandle);
                    return;
                }

                // Otherwise, convert the results.
                TArray<FOSSInterfaceListEntry> Entries;
                TArray<FCloudFileHeader> UserCloudFiles;
                UserCloud->GetUserFileList(*UserId, UserCloudFiles);
                for (const auto &UserCloudFile : UserCloudFiles)
                {
                    FOSSInterfaceListEntry Entry;
                    Entry.Id = UserCloudFile.FileName;
                    Entry.DisplayName = FText::FromString(UserCloudFile.FileName);
                    Entries.Add(Entry);
                }

                // Return the results.
                ResultWk->OnResult(true, Entries, TEXT(""));

                // Unregister this callback since we've handled the call we care about.
                UserCloud->ClearOnEnumerateUserFilesCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Start the enumeration of user files.
    UserCloud->EnumerateUserFiles(*UserId);
}

void UMOS_GameInstanceSubsystem::ExecuteUserCloudWriteStringToFile(
    const FString &FileName,
    const FString &FileContents,
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

    // Get the user cloud interface, if the online subsystem supports it.
    auto UserCloud = OSS->GetUserCloudInterface();
    if (!UserCloud.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support user cloud."));
        return;
    }

    // Convert UTF8 string data to bytes.
    FTCHARToUTF8 Data(*FileContents);
    TArray<uint8> Bytes(reinterpret_cast<const uint8 *>(Data.Get()), Data.Length());

    // Register an event so we can receive the outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle =
        UserCloud->AddOnWriteUserFileCompleteDelegate_Handle(FOnWriteUserFileCompleteDelegate::CreateWeakLambda(
            this,
            [UserCloud, CallbackHandle, ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result), FileName, UserId](
                bool bCallbackWasSuccessful,
                const FUniqueNetId &CallbackUserId,
                const FString &CallbackFileName) {
                // Check if this callback is for us.
                if (*UserId != CallbackUserId || FileName != CallbackFileName)
                {
                    // This callback isn't for our call.
                    return;
                }

                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return the result.
                ResultWk->OnResult(
                    bCallbackWasSuccessful,
                    bCallbackWasSuccessful ? TEXT("") : TEXT("WriteUserFile call failed."));

                // Unregister this callback since we've handled the call we care about.
                UserCloud->ClearOnWriteUserFileCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Start writing the file.
    if (!UserCloud->WriteUserFile(*UserId, FileName, Bytes, false))
    {
        Result->OnResult(false, TEXT("WriteUserFile call failed to start."));
        UserCloud->ClearOnWriteUserFileCompleteDelegate_Handle(*CallbackHandle);
    }
}

void UMOS_GameInstanceSubsystem::ExecuteUserCloudWriteSaveGameToFile(
    const FString &FileName,
    double SaveGameNumber,
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

    // Get the user cloud interface, if the online subsystem supports it.
    auto UserCloud = OSS->GetUserCloudInterface();
    if (!UserCloud.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support user cloud."));
        return;
    }

    // Create the save game, store the float in it, and serialize.
    UMOS_SaveGame *SaveGame = NewObject<UMOS_SaveGame>();
    SaveGame->StoredFloat = (float)SaveGameNumber;
    TArray<uint8> SaveData;
    if (!UGameplayStatics::SaveGameToMemory(SaveGame, SaveData))
    {
        Result->OnResult(false, TEXT("Failed to serialize save game object."));
        return;
    }

    // Register an event so we can receive the outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle =
        UserCloud->AddOnWriteUserFileCompleteDelegate_Handle(FOnWriteUserFileCompleteDelegate::CreateWeakLambda(
            this,
            [UserCloud, CallbackHandle, ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result), FileName, UserId](
                bool bCallbackWasSuccessful,
                const FUniqueNetId &CallbackUserId,
                const FString &CallbackFileName) {
                // Check if this callback is for us.
                if (*UserId != CallbackUserId || FileName != CallbackFileName)
                {
                    // This callback isn't for our call.
                    return;
                }

                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return the result.
                ResultWk->OnResult(
                    bCallbackWasSuccessful,
                    bCallbackWasSuccessful ? TEXT("") : TEXT("WriteUserFile call failed."));

                // Unregister this callback since we've handled the call we care about.
                UserCloud->ClearOnWriteUserFileCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Start writing the file.
    if (!UserCloud->WriteUserFile(*UserId, FileName, SaveData, false))
    {
        Result->OnResult(false, TEXT("WriteUserFile call failed to start."));
        UserCloud->ClearOnWriteUserFileCompleteDelegate_Handle(*CallbackHandle);
    }
}

void UMOS_GameInstanceSubsystem::ExecuteUserCloudReadStringFromFile(
    const FString &FileName,
    UMOS_ReadFileStringAsyncResult *Result)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        Result->OnResult(false, TEXT(""), TEXT("Online subsystem is not available."));
        return;
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the user cloud interface, if the online subsystem supports it.
    auto UserCloud = OSS->GetUserCloudInterface();
    if (!UserCloud.IsValid())
    {
        Result->OnResult(false, TEXT(""), TEXT("Online subsystem does not support user cloud."));
        return;
    }

    // Register an event so we can receive the outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle =
        UserCloud->AddOnReadUserFileCompleteDelegate_Handle(FOnReadUserFileCompleteDelegate::CreateWeakLambda(
            this,
            [UserCloud,
             CallbackHandle,
             ResultWk = TSoftObjectPtr<UMOS_ReadFileStringAsyncResult>(Result),
             FileName,
             UserId](bool bCallbackWasSuccessful, const FUniqueNetId &CallbackUserId, const FString &CallbackFileName) {
                // Check if this callback is for us.
                if (*UserId != CallbackUserId || FileName != CallbackFileName)
                {
                    // This callback isn't for our call.
                    return;
                }

                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // If the read failed, retur now.
                if (!bCallbackWasSuccessful)
                {
                    ResultWk->OnResult(false, TEXT(""), TEXT("ReadUserFile call failed."));
                    UserCloud->ClearOnReadUserFileCompleteDelegate_Handle(*CallbackHandle);
                    return;
                }

                // Get the file contents.
                TArray<uint8> FileContents;
                if (!UserCloud->GetFileContents(*UserId, FileName, FileContents))
                {
                    ResultWk->OnResult(false, TEXT(""), TEXT("GetFileContents call failed."));
                    UserCloud->ClearOnReadUserFileCompleteDelegate_Handle(*CallbackHandle);
                    return;
                }

                // Return the result.
                FUTF8ToTCHAR Data(reinterpret_cast<const ANSICHAR *>(FileContents.GetData()), FileContents.Num());
                ResultWk->OnResult(true, FString(Data.Length(), Data.Get()), TEXT(""));

                // Unregister this callback since we've handled the call we care about.
                UserCloud->ClearOnReadUserFileCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Start reading the file.
    if (!UserCloud->ReadUserFile(*UserId, FileName))
    {
        Result->OnResult(false, TEXT(""), TEXT("ReadUserFile call failed to start."));
        UserCloud->ClearOnReadUserFileCompleteDelegate_Handle(*CallbackHandle);
    }
}

void UMOS_GameInstanceSubsystem::ExecuteUserCloudReadSaveGameFromFile(
    const FString &FileName,
    UMOS_ReadFileSaveGameAsyncResult *Result)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        Result->OnResult(false, 0.0, TEXT("Online subsystem is not available."));
        return;
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the user cloud interface, if the online subsystem supports it.
    auto UserCloud = OSS->GetUserCloudInterface();
    if (!UserCloud.IsValid())
    {
        Result->OnResult(false, 0.0, TEXT("Online subsystem does not support user cloud."));
        return;
    }

    // Register an event so we can receive the outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle =
        UserCloud->AddOnReadUserFileCompleteDelegate_Handle(FOnReadUserFileCompleteDelegate::CreateWeakLambda(
            this,
            [UserCloud,
             CallbackHandle,
             ResultWk = TSoftObjectPtr<UMOS_ReadFileSaveGameAsyncResult>(Result),
             FileName,
             UserId](bool bCallbackWasSuccessful, const FUniqueNetId &CallbackUserId, const FString &CallbackFileName) {
                // Check if this callback is for us.
                if (*UserId != CallbackUserId || FileName != CallbackFileName)
                {
                    // This callback isn't for our call.
                    return;
                }

                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // If the read failed, retur now.
                if (!bCallbackWasSuccessful)
                {
                    ResultWk->OnResult(false, 0.0, TEXT("ReadUserFile call failed."));
                    UserCloud->ClearOnReadUserFileCompleteDelegate_Handle(*CallbackHandle);
                    return;
                }

                // Get the file contents.
                TArray<uint8> FileContents;
                if (!UserCloud->GetFileContents(*UserId, FileName, FileContents))
                {
                    ResultWk->OnResult(false, 0.0, TEXT("GetFileContents call failed."));
                    UserCloud->ClearOnReadUserFileCompleteDelegate_Handle(*CallbackHandle);
                    return;
                }

                // Return the result.
                UMOS_SaveGame *SaveGame =
                    Cast<UMOS_SaveGame>(UGameplayStatics::LoadGameFromMemory(FileContents));
                if (!IsValid(SaveGame))
                {
                    ResultWk->OnResult(false, 0.0, TEXT("Unable to deserialize memory to USaveGame."));
                    UserCloud->ClearOnReadUserFileCompleteDelegate_Handle(*CallbackHandle);
                    return;
                }
                ResultWk->OnResult(true, static_cast<double>(SaveGame->StoredFloat), TEXT(""));

                // Unregister this callback since we've handled the call we care about.
                UserCloud->ClearOnReadUserFileCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Start reading the file.
    if (!UserCloud->ReadUserFile(*UserId, FileName))
    {
        Result->OnResult(false, 0.0, TEXT("ReadUserFile call failed to start."));
        UserCloud->ClearOnReadUserFileCompleteDelegate_Handle(*CallbackHandle);
    }
}