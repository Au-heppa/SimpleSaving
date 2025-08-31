// Copyright Tero "Au-heppa" Knuutinen 2025.
// Free to use for any personal project or company with less than 13 employees
// Do not use to train AI / LLM / neural network

#pragma once

#include "SavedTime.generated.h"

//=================================================================
// 
//=================================================================
USTRUCT(BlueprintType)
struct FSavedTime
{
	GENERATED_USTRUCT_BODY()

	//
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Time = 0.0f;

	//====================================================================================================
	//
	//====================================================================================================

	//
	inline FSavedTime & operator=(float InTime)
	{
		Time = InTime;
		return *this;
	}

	//
	inline FSavedTime & operator=(double InTime)
	{
		Time = InTime;
		return *this;
	}

	//
	inline FSavedTime & operator=(int32 InTime)
	{
		Time = InTime;
		return *this;
	}

	//====================================================================================================
	//
	//====================================================================================================

	//
	UE_NODISCARD FORCEINLINE friend bool operator<=(const FSavedTime& Lhs, const FSavedTime& Rhs)
	{
		return Lhs.Time <= Rhs.Time;
	}

	//
	UE_NODISCARD FORCEINLINE friend bool operator<=(const FSavedTime& Lhs, float Rhs)
	{
		return Lhs.Time <= Rhs;
	}

	//
	UE_NODISCARD FORCEINLINE friend bool operator<=(const FSavedTime& Lhs, double Rhs)
	{
		return Lhs.Time <= Rhs;
	}

	//
	UE_NODISCARD FORCEINLINE friend bool operator<=(float Lhs, const FSavedTime& Rhs)
	{
		return Lhs <= Rhs.Time;
	}

	//
	UE_NODISCARD FORCEINLINE friend bool operator<=(double Lhs, const FSavedTime& Rhs)
	{
		return Lhs <= Rhs.Time;
	}

	//====================================================================================================
	//
	//====================================================================================================

	//
	UE_NODISCARD FORCEINLINE friend bool operator<(const FSavedTime& Lhs, const FSavedTime& Rhs)
	{
		return Lhs.Time < Rhs.Time;
	}

	//
	UE_NODISCARD FORCEINLINE friend bool operator<(const FSavedTime& Lhs, float Rhs)
	{
		return Lhs.Time < Rhs;
	}

	//
	UE_NODISCARD FORCEINLINE friend bool operator<(const FSavedTime& Lhs, double Rhs)
	{
		return Lhs.Time < Rhs;
	}

	//
	UE_NODISCARD FORCEINLINE friend bool operator<(float Lhs, const FSavedTime& Rhs)
	{
		return Lhs < Rhs.Time;
	}

	//
	UE_NODISCARD FORCEINLINE friend bool operator<(double Lhs, const FSavedTime& Rhs)
	{
		return Lhs < Rhs.Time;
	}

	//====================================================================================================
	//
	//====================================================================================================

	//
	UE_NODISCARD FORCEINLINE friend bool operator>=(const FSavedTime& Lhs, const FSavedTime & Rhs)
	{
		return Lhs.Time >= Rhs.Time;
	}

	//
	UE_NODISCARD FORCEINLINE friend bool operator>=(const FSavedTime& Lhs, float Rhs)
	{
		return Lhs.Time >= Rhs;
	}

	//
	UE_NODISCARD FORCEINLINE friend bool operator>=(const FSavedTime& Lhs, double Rhs)
	{
		return Lhs.Time >= Rhs;
	}

	//
	UE_NODISCARD FORCEINLINE friend bool operator>=(float Lhs, const FSavedTime& Rhs)
	{
		return Lhs >= Rhs.Time;
	}

	//
	UE_NODISCARD FORCEINLINE friend bool operator>=(double Lhs, const FSavedTime& Rhs)
	{
		return Lhs >= Rhs.Time;
	}

	//====================================================================================================
	//
	//====================================================================================================

	//
	UE_NODISCARD FORCEINLINE friend bool operator>(const FSavedTime& Lhs, const FSavedTime & Rhs)
	{
		return Lhs.Time > Rhs.Time;
	}

	//
	UE_NODISCARD FORCEINLINE friend bool operator>(const FSavedTime& Lhs, float Rhs)
	{
		return Lhs.Time > Rhs;
	}

	//
	UE_NODISCARD FORCEINLINE friend bool operator>(const FSavedTime& Lhs, double Rhs)
	{
		return Lhs.Time > Rhs;
	}

	//
	UE_NODISCARD FORCEINLINE friend bool operator>(float Lhs, const FSavedTime& Rhs)
	{
		return Lhs > Rhs.Time;
	}

	//
	UE_NODISCARD FORCEINLINE friend bool operator>(double Lhs, const FSavedTime& Rhs)
	{
		return Lhs > Rhs.Time;
	}

	//====================================================================================================
	//
	//====================================================================================================

	//
	UE_NODISCARD FORCEINLINE friend bool operator!=(const FSavedTime& Lhs, const FSavedTime & Rhs)
	{
		return Lhs.Time != Rhs.Time;
	}

	//
	UE_NODISCARD FORCEINLINE friend bool operator!=(const FSavedTime& Lhs, float Rhs)
	{
		return Lhs.Time != Rhs;
	}

	//
	UE_NODISCARD FORCEINLINE friend bool operator!=(const FSavedTime& Lhs, double Rhs)
	{
		return Lhs.Time != Rhs;
	}

	//
	UE_NODISCARD FORCEINLINE friend bool operator!=(float Lhs, const FSavedTime& Rhs)
	{
		return Lhs != Rhs.Time;
	}

	//
	UE_NODISCARD FORCEINLINE friend bool operator!=(double Lhs, const FSavedTime& Rhs)
	{
		return Lhs != Rhs.Time;
	}

	//====================================================================================================
	//
	//====================================================================================================

	//
	UE_NODISCARD FORCEINLINE friend bool operator==(const FSavedTime& Lhs, const FSavedTime & Rhs)
	{
		return Lhs.Time == Rhs.Time;
	}

	//
	UE_NODISCARD FORCEINLINE friend bool operator==(const FSavedTime& Lhs, float Rhs)
	{
		return Lhs.Time == Rhs;
	}

	//
	UE_NODISCARD FORCEINLINE friend bool operator==(const FSavedTime& Lhs, double Rhs)
	{
		return Lhs.Time == Rhs;
	}

	//
	UE_NODISCARD FORCEINLINE friend bool operator==(float Lhs, const FSavedTime& Rhs)
	{
		return Lhs == Rhs.Time;
	}

	//
	UE_NODISCARD FORCEINLINE friend bool operator==(double Lhs, const FSavedTime& Rhs)
	{
		return Lhs == Rhs.Time;
	}


	//====================================================================================================
	//
	//====================================================================================================

	//
	UE_NODISCARD FORCEINLINE friend double operator+(const FSavedTime& Lhs, const FSavedTime& Rhs)
	{
		return Lhs.Time + Rhs.Time;
	}

	//
	UE_NODISCARD FORCEINLINE friend double operator+(const FSavedTime& Lhs, float Rhs)
	{
		return Lhs.Time + Rhs;
	}

	//
	UE_NODISCARD FORCEINLINE friend double operator+(const FSavedTime& Lhs, double Rhs)
	{
		return Lhs.Time + Rhs;
	}

	//
	UE_NODISCARD FORCEINLINE friend double operator+(float Lhs, const FSavedTime& Rhs)
	{
		return Lhs + Rhs.Time;
	}

	//
	UE_NODISCARD FORCEINLINE friend double operator+(double Lhs, const FSavedTime& Rhs)
	{
		return Lhs + Rhs.Time;
	}
};