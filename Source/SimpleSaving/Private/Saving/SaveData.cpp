// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#include "Saving/SaveData.h"

//==============================================================================================================
//
//==============================================================================================================
int32 FCustomSaveData::_CalculateBytes(const TMap<FName, FString>& InSingles, const TMap<FName, FArrayData>& InArrays, const TMap<FName, FMapData>& InMaps )
{
	int32 iTotal = 0;

	iTotal += sizeof(int32);
	for (auto It = InSingles.CreateConstIterator(); It; ++It)
	{
		iTotal += It.Key().ToString().GetAllocatedSize() + sizeof(int32);
		iTotal += It.Value().GetAllocatedSize() + sizeof(int32);
	}

	iTotal += sizeof(int32);
	for (auto It = InArrays.CreateConstIterator(); It; ++It)
	{
		iTotal += It.Key().ToString().GetAllocatedSize() + sizeof(int32);
		iTotal += sizeof(int32); // It.Value().Data.GetAllocatedSize();

		for (int32 i = 0; i < It.Value().Data.Num(); i++)
		{
			iTotal += It.Value().Data.GetData()[i].GetAllocatedSize() + sizeof(int32);
		}
	}

	iTotal += sizeof(int32);
	for (auto It = InMaps.CreateConstIterator(); It; ++It)
	{
		iTotal += It.Key().ToString().GetAllocatedSize() + sizeof(int32);
		//iTotal += It.Value().Data.GetAllocatedSize();

		iTotal += sizeof(int32); // It.Value().Data.GetAllocatedSize();

		for (auto It2 = It.Value().Data.CreateConstIterator(); It2; ++It2)
		{
			iTotal += It2.Key().GetAllocatedSize() + sizeof(int32);
			iTotal += It2.Value().GetAllocatedSize() + sizeof(int32);
		}
	}

	return iTotal;
}

//==============================================================================================================
//
//==============================================================================================================
void FCustomSaveData::_GetSize(	const TMap<FName, FString> &InSingles, 
								const TMap<FName, FArrayData> &InArrays, 
								const TMap<FName, FMapData> &InMaps,
								int32 &OutMegabytes, 
								int32 &OutKilobytes, 
								int32 &OutBytes )
{
	int32 iTotal = _CalculateBytes(InSingles, InArrays, InMaps);

	_ParseBytes(iTotal, OutMegabytes, OutKilobytes, OutBytes);
}

//==============================================================================================================
//
//==============================================================================================================
void FCustomSaveData::_ParseBytes(int32 InBytes, int32& OutMegabytes, int32& OutKilobytes, int32& OutBytes)
{
	OutBytes = InBytes;

	OutKilobytes = OutBytes / 1024;
	OutBytes = OutBytes - (OutKilobytes * 1024);

	OutMegabytes = OutKilobytes / 1024;
	OutKilobytes = OutKilobytes - (OutMegabytes * 1024);
}

//==============================================================================================================
//
//==============================================================================================================
void FCustomSaveData::_GetSizeString(	int32 InMegabytes,
										int32 InKilobytes,
										int32 InBytes,
										FString &OutString )
{
	if (InMegabytes > 0)
	{
		float flFraction = (float)InKilobytes / 1024.0f;
		int32 iFrac = flFraction * 10.0f;

		OutString = FString::Printf(TEXT("%d.%d MB"), InMegabytes, iFrac);
	}
	else if (InKilobytes > 0)
	{
		float flFraction = (float)InBytes / 1024.0f;
		int32 iFrac = flFraction * 10.0f;

		OutString = FString::Printf(TEXT("%d.%d KB"), InKilobytes, iFrac);
	}
	else
	{
		OutString = FString::Printf(TEXT("%d b"), InBytes);
	}
}