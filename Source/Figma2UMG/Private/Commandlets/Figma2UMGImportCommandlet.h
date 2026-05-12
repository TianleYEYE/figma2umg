// Copyright 2024 Buvi Games. All Rights Reserved.

#pragma once

#include "Commandlets/Commandlet.h"
#include "FigmaImportSubsystem.h"

#include "Figma2UMGImportCommandlet.generated.h"

UCLASS()
class UFigma2UMGImportCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UFigma2UMGImportCommandlet();

	virtual int32 Main(const FString& Params) override;

private:
	void HandleImportStatus(eRequestStatus Status, FString Message);

	bool bImportFinished = false;
	bool bImportSucceeded = false;
	FString LastStatusMessage;
};
