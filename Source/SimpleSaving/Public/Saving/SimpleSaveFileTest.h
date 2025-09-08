#pragma once

#include "CoreMinimal.h"
#include "SimpleSaveFileTest.generated.h"

//=================================================================
// Class for testing saving
//=================================================================
UCLASS()
class USaveTestObject : public UObject
{
	GENERATED_BODY()

public:

	void Randomize();
	bool Matches(class USaveTestObject *InOther) const;

	UPROPERTY(SaveGame)
	int32 SomeInteger = 0;

	UPROPERTY(SaveGame)
	float SomeFloat = 0.0f;

	UPROPERTY(SaveGame)
	TArray<int32> SomeArray;

	UPROPERTY(SaveGame)
	TMap<int32, float> SomeMap;
};