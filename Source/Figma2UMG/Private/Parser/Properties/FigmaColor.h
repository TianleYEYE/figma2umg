// Copyright 2024 Buvi Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "FigmaColor.generated.h"

USTRUCT()
struct FIGMA2UMG_API FFigmaColor
{
public:
	GENERATED_BODY()

	FLinearColor ToLinearColor(float Opacity = 1.0f) const
	{
		const FColor SRGBColor(
			FMath::RoundToInt(FMath::Clamp(R, 0.0f, 1.0f) * 255.0f),
			FMath::RoundToInt(FMath::Clamp(G, 0.0f, 1.0f) * 255.0f),
			FMath::RoundToInt(FMath::Clamp(B, 0.0f, 1.0f) * 255.0f),
			FMath::RoundToInt(FMath::Clamp(A * Opacity, 0.0f, 1.0f) * 255.0f)
		);

		return FLinearColor::FromSRGBColor(SRGBColor);
	}

	UPROPERTY()
	float R = 1.0f;

	UPROPERTY()
	float G = 1.0f;

	UPROPERTY()
	float B = 1.0f;

	UPROPERTY()
	float A = 1.0f;
};
