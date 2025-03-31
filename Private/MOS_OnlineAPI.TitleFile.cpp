// Copyright June Rhodes. MIT Licensed.

#include "MultiplayerOnlineSusbsystem/Public/MOS_GameInstanceSubsystem.h"

#include "Interfaces/OnlineTitleFileInterface.h"
#include "OnlineSubsystemUtils.h"


void UMOS_GameInstanceSubsystem::ExecuteTitleFileQueryFiles(UMOS_ListAsyncResult *Result)
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

    // Get the title file interface, if the online subsystem supports it.
    auto TitleFile = OSS->GetTitleFileInterface();
    if (!TitleFile.IsValid())
    {
        Result->OnResult(
            false,
            TArray<FOSSInterfaceListEntry>(),
            TEXT("Online subsystem does not support title file."));
        return;
    }

    // Register an event so we can receive the outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle =
        TitleFile->AddOnEnumerateFilesCompleteDelegate_Handle(FOnEnumerateFilesCompleteDelegate::CreateWeakLambda(
            this,
            [TitleFile, CallbackHandle, ResultWk = TSoftObjectPtr<UMOS_ListAsyncResult>(Result), UserId](
                bool bCallbackWasSuccessful,
                const FString &CallbackErrorMessage) {
                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return if the read failed.
                if (!bCallbackWasSuccessful)
                {
                    ResultWk->OnResult(false, TArray<FOSSInterfaceListEntry>(), CallbackErrorMessage);
                    TitleFile->ClearOnEnumerateFilesCompleteDelegate_Handle(*CallbackHandle);
                    return;
                }

                // Otherwise, convert the results.
                TArray<FOSSInterfaceListEntry> Entries;
                TArray<FCloudFileHeader> TitleFileFiles;
                TitleFile->GetFileList(TitleFileFiles);
                for (const auto &TitleFileFile : TitleFileFiles)
                {
                    FOSSInterfaceListEntry Entry;
                    Entry.Id = TitleFileFile.FileName;
                    Entry.DisplayName = FText::FromString(TitleFileFile.FileName);
                    Entries.Add(Entry);
                }

                // Return the results.
                ResultWk->OnResult(true, Entries, TEXT(""));

                // Unregister this callback since we've handled the call we care about.
                TitleFile->ClearOnEnumerateFilesCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Start the enumeration of title files.
    TitleFile->EnumerateFiles(FPagedQuery(0, -1));
}

void UMOS_GameInstanceSubsystem::ExecuteTitleFileReadStringFromFile(
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

    // Get the title file interface, if the online subsystem supports it.
    auto TitleFile = OSS->GetTitleFileInterface();
    if (!TitleFile.IsValid())
    {
        Result->OnResult(false, TEXT(""), TEXT("Online subsystem does not support title file."));
        return;
    }

    // Register an event so we can receive the outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = TitleFile->AddOnReadFileCompleteDelegate_Handle(FOnReadFileCompleteDelegate::CreateWeakLambda(
        this,
        [TitleFile,
         CallbackHandle,
         ResultWk = TSoftObjectPtr<UMOS_ReadFileStringAsyncResult>(Result),
         FileName,
         UserId](bool bCallbackWasSuccessful, const FString &CallbackFileName) {
            // Check if this callback is for us.
            if (FileName != CallbackFileName)
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
                ResultWk->OnResult(false, TEXT(""), TEXT("ReadFile call failed."));
                TitleFile->ClearOnReadFileCompleteDelegate_Handle(*CallbackHandle);
                return;
            }

            // Get the file contents.
            TArray<uint8> FileContents;
            if (!TitleFile->GetFileContents(FileName, FileContents))
            {
                ResultWk->OnResult(false, TEXT(""), TEXT("GetFileContents call failed."));
                TitleFile->ClearOnReadFileCompleteDelegate_Handle(*CallbackHandle);
                return;
            }

            // Return the result.
            FUTF8ToTCHAR Data(reinterpret_cast<const ANSICHAR *>(FileContents.GetData()), FileContents.Num());
            ResultWk->OnResult(true, FString(Data.Length(), Data.Get()), TEXT(""));

            // Unregister this callback since we've handled the call we care about.
            TitleFile->ClearOnReadFileCompleteDelegate_Handle(*CallbackHandle);
        }));

    // Start reading the file.
    if (!TitleFile->ReadFile(FileName))
    {
        Result->OnResult(false, TEXT(""), TEXT("ReadFile call failed to start."));
        TitleFile->ClearOnReadFileCompleteDelegate_Handle(*CallbackHandle);
    }
}