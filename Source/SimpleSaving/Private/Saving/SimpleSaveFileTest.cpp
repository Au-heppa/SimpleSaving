#include "Saving/SimpleSaveFileTest.h"
#include "Saving/SimpleSaveFile.h"
#include "AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSimplePropertiesTest, "SimpleSaving.SimpleProperties", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)


//=========================================================================================================================
// 
//=========================================================================================================================
void USaveTestObject::Randomize()
{
	SomeInteger = FMath::RandRange(0,100);
	SomeFloat = FMath::RandRange(0.0f, 100.0f);

	SomeArray.SetNum(FMath::RandRange(1,5));
	for (int32 i=0; i<SomeArray.Num(); i++)
	{
		SomeArray.GetData()[i] = FMath::RandRange(0, 100);
	}

	SomeMap.Reset();
	int32 iNum = FMath::RandRange(1,5);
	for (int32 i = 0; i < iNum; i++)
	{
		SomeMap.Emplace(FMath::RandRange(0, 100), FMath::RandRange(0.0f, 100.0f) );
	}
}

//=========================================================================================================================
// 
//=========================================================================================================================
bool USaveTestObject::Matches(class USaveTestObject* InOther) const
{
	if (FMath::Abs(SomeFloat - InOther->SomeFloat) > KINDA_SMALL_NUMBER)
	{
		UE_LOG(LogTemp, Error, TEXT("Float does not match: %f and %f"), SomeFloat, InOther->SomeFloat);
		return false;
	}

	if (SomeInteger != InOther->SomeInteger)
	{
		UE_LOG(LogTemp, Error, TEXT("Integer does not match!"));
		return false;
	}

	if (SomeArray != InOther->SomeArray)
	{
		UE_LOG(LogTemp, Error, TEXT("Array does not match: Sizes %d and %d!"), SomeArray.Num(), InOther->SomeArray.Num());
		return false;
	}

	for (int32 i=0; i<SomeArray.Num(); i++)
	{
		if (SomeArray.GetData()[i] != InOther->SomeArray.GetData()[i])
		{
			UE_LOG(LogTemp, Error, TEXT("Array does not match: Index %d values %d and %d"), i, SomeArray.GetData()[i], InOther->SomeArray.GetData()[i]);
			return false;
		}
	}

	for (auto It = SomeMap.CreateConstIterator(); It; ++It)
	{
		if (!InOther->SomeMap.Contains(It.Key()))
		{
			UE_LOG(LogTemp, Error, TEXT("Map does not match: Other does not contain key %d"), It.Key());
			return false;
		}

		if (FMath::Abs(It.Value() - InOther->SomeMap[It.Key()]) > KINDA_SMALL_NUMBER)
		{
			UE_LOG(LogTemp, Error, TEXT("Map does not match: Key %d mismatch %f and %f"), It.Key(), It.Value(), InOther->SomeMap[It.Key()]);
			return false;
		}
	}

	return true;
}

//=========================================================================================================================
// 
//=========================================================================================================================
bool FSimplePropertiesTest::RunTest(const FString& Parameters)
{
	class USaveTestObject *pTestObject1 = NewObject<USaveTestObject>();
	pTestObject1->Randomize();

	class USaveTestObject *pTestObject2 = DuplicateObject(pTestObject1, pTestObject1->GetOuter());

	UE_LOG(LogTemp, Display, TEXT("Test integer %d Float %.2f Array %d"), pTestObject1->SomeInteger, pTestObject1->SomeFloat, pTestObject1->SomeArray.Num());

	FSimpleSaveData Data;
	USimpleSaveFile::SaveSimpleProperties(pTestObject1, Data, false);

	pTestObject1->Randomize();

	USimpleSaveFile::LoadSimpleProperties(pTestObject1, Data);

	// Make the test pass by returning true, or fail by returning false.
	return pTestObject1->Matches(pTestObject2);
}