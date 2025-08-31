// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#include "Saving/SaveInterface.h"

//=================================================================
// 
//=================================================================
USaveInterface::USaveInterface(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{

}

//============================================================================================
//
//============================================================================================
FName ISaveInterface::EnsureUniqueSavingTag(class AActor *InActor, const FName &InSavingTag, const TCHAR *InDefaultName)
{
	if (IsValid(InActor) == false)
		return InSavingTag;

	if (IsValid(InActor->GetWorld()) == false)
		return InSavingTag;

	TArray<class AActor*> Actors;
	TArray<class ISaveInterface*> Interfaces;
	UGameplayStatics::GetAllActorsWithInterface(InActor, USaveInterface::StaticClass(), Actors);

	for (int32 i=0; i<Actors.Num(); i++)
	{
		class ISaveInterface *pInterface = Cast<ISaveInterface>(Actors.GetData()[i]);
		if (pInterface && Actors.GetData()[i] != InActor)
		{
			Interfaces.Add(pInterface);
		}
	}

	if (!InSavingTag.IsNone())
	{
		bool bUsed = false;
		for (int32 i=0; i<Interfaces.Num(); i++)
		{
			class ISaveInterface *pInterface = Interfaces.GetData()[i];
			if (pInterface && pInterface->GetSavingTag() == InSavingTag)
			{
				bUsed = true;
				break;
			}
		}

		if (!bUsed)
			return InSavingTag;
	}

	FString NameToUse;
	if (!InSavingTag.IsNone())
	{
		NameToUse = InSavingTag.ToString();

		FString Left;
		if (NameToUse.Split(TEXT("_"), &Left, NULL, ESearchCase::IgnoreCase, ESearchDir::FromEnd) && Left.Len() > 0)
		{
			NameToUse = Left;
		}
	}
	else
	{
		NameToUse = InDefaultName;
	}

	TArray<FString> Names;
	Names.SetNum(Interfaces.Num());
	for (int32 i=0; i<Interfaces.Num(); i++)
	{
		Names.GetData()[i] = Interfaces.GetData()[i]->GetSavingTag().ToString();
	}

	int32 iAttempt = 1;

	while (true)
	{
		FString NameAttempt = FString::Printf(TEXT("%s_%d"), *NameToUse, iAttempt);

		bool bFound = false;

		for (int32 i=0; i<Names.Num(); i++)
		{
			if (Names.GetData()[i].Equals(NameAttempt, ESearchCase::IgnoreCase))
			{
				bFound = true;
				break;
			}
		}

		if (bFound)
		{
			iAttempt++;
			continue;
		}

		return *NameAttempt;
	}

	return NAME_None;
}
