// Copyright 2024 Buvi Games. All Rights Reserved.

#include "Commandlets/Figma2UMGImportCommandlet.h"

#include "Figma2UMGModule.h"
#include "HttpManager.h"
#include "HttpModule.h"
#include "REST/RequestParams.h"
#include "Async/TaskGraphInterfaces.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Parse.h"

UFigma2UMGImportCommandlet::UFigma2UMGImportCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
	ShowErrorCount = true;
}

int32 UFigma2UMGImportCommandlet::Main(const FString& Params)
{
	FString AccessToken;
	FString FileKey;
	FString ContentRootFolder;
	FString Ids;
	FString LibraryFileKeys;
	FString MaxURLImageRequestString;
	FString RequestDelaySecondsString;
	FString TimeoutSecondsString;

	FParse::Value(*Params, TEXT("FigmaToken="), AccessToken);
	FParse::Value(*Params, TEXT("FigmaFileKey="), FileKey);
	FParse::Value(*Params, TEXT("FigmaContentRoot="), ContentRootFolder);
	FParse::Value(*Params, TEXT("FigmaIds="), Ids);
	FParse::Value(*Params, TEXT("FigmaLibraryFileKeys="), LibraryFileKeys);
	FParse::Value(*Params, TEXT("FigmaMaxURLImageRequest="), MaxURLImageRequestString);
	FParse::Value(*Params, TEXT("FigmaRequestDelaySeconds="), RequestDelaySecondsString);
	FParse::Value(*Params, TEXT("FigmaTimeoutSeconds="), TimeoutSecondsString);

	if (AccessToken.IsEmpty() || FileKey.IsEmpty())
	{
		UE_LOG_Figma2UMG(Error, TEXT("Usage: -run=Figma2UMGImport -FigmaToken=<token> -FigmaFileKey=<key> [-FigmaContentRoot=/Game/FigmaImport] [-FigmaIds=1:2,3:4] [-FigmaLibraryFileKeys=<key,key>] [-FigmaSaveAll] [-FigmaTimeoutSeconds=600]"));
		return 1;
	}

	if (ContentRootFolder.IsEmpty())
	{
		ContentRootFolder = TEXT("/Game/FigmaImport");
	}

	float TimeoutSeconds = 600.0f;
	if (!TimeoutSecondsString.IsEmpty())
	{
		TimeoutSeconds = FCString::Atof(*TimeoutSecondsString);
	}

	URequestParams* RequestParams = NewObject<URequestParams>(GetTransientPackage());
	RequestParams->AddToRoot();
	RequestParams->AccessToken = AccessToken;
	RequestParams->FileKey = FileKey;
	RequestParams->ContentRootFolder = ContentRootFolder;
	RequestParams->DownloadFontsFromGoogle = false;
	RequestParams->UsePrototypeFlow = FParse::Param(*Params, TEXT("FigmaUsePrototypeFlow"));
	RequestParams->SaveAllAtEnd = FParse::Param(*Params, TEXT("FigmaSaveAll"));
	RequestParams->ProgressOnFailToDownloadImage = FParse::Param(*Params, TEXT("FigmaProgressOnFailedImage"));

	if (!MaxURLImageRequestString.IsEmpty())
	{
		RequestParams->MaxURLImageRequest = FMath::Clamp(FCString::Atoi(*MaxURLImageRequestString), 1, 30);
	}

	if (!RequestDelaySecondsString.IsEmpty())
	{
		RequestParams->RequestDelaySeconds = FMath::Clamp(FCString::Atof(*RequestDelaySecondsString), 0.0f, 30.0f);
	}

	if (!Ids.IsEmpty())
	{
		Ids.ParseIntoArray(RequestParams->Ids, TEXT(","), true);
	}

	if (!LibraryFileKeys.IsEmpty())
	{
		LibraryFileKeys.ParseIntoArray(RequestParams->LibraryFileKeys, TEXT(","), true);
	}

	UFigmaImportSubsystem* ImportSubsystem = GEditor ? GEditor->GetEditorSubsystem<UFigmaImportSubsystem>() : nullptr;
	if (!ImportSubsystem)
	{
		UE_LOG_Figma2UMG(Error, TEXT("Figma import subsystem is not available."));
		RequestParams->RemoveFromRoot();
		return 1;
	}

	bImportFinished = false;
	bImportSucceeded = false;
	LastStatusMessage.Reset();

	UE_LOG_Figma2UMG(Display, TEXT("Starting Figma2UMG commandlet import for file %s into %s."), *FileKey, *ContentRootFolder);
	ImportSubsystem->Request(RequestParams, FOnFigmaImportUpdateStatusCB::CreateUObject(this, &UFigma2UMGImportCommandlet::HandleImportStatus));

	const double StartTime = FPlatformTime::Seconds();
	double LastTickTime = StartTime;
	while (!bImportFinished)
	{
		const double CurrentTime = FPlatformTime::Seconds();
		const float DeltaTime = static_cast<float>(CurrentTime - LastTickTime);
		LastTickTime = CurrentTime;

		FHttpModule::Get().GetHttpManager().Tick(DeltaTime);
		FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);

		if ((CurrentTime - StartTime) > TimeoutSeconds)
		{
			LastStatusMessage = FString::Printf(TEXT("Timed out after %.1f seconds."), TimeoutSeconds);
			UE_LOG_Figma2UMG(Error, TEXT("%s"), *LastStatusMessage);
			break;
		}

		FPlatformProcess::Sleep(0.03f);
	}

	FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
	RequestParams->RemoveFromRoot();

	if (bImportSucceeded)
	{
		UE_LOG_Figma2UMG(Display, TEXT("Figma2UMG commandlet import succeeded: %s"), *LastStatusMessage);
		return 0;
	}

	UE_LOG_Figma2UMG(Error, TEXT("Figma2UMG commandlet import failed: %s"), *LastStatusMessage);
	return 1;
}

void UFigma2UMGImportCommandlet::HandleImportStatus(eRequestStatus Status, FString Message)
{
	LastStatusMessage = Message;

	if (Status == eRequestStatus::Processing)
	{
		UE_LOG_Figma2UMG(Display, TEXT("%s"), *Message);
		return;
	}

	bImportFinished = true;
	bImportSucceeded = Status == eRequestStatus::Succeeded;
}
