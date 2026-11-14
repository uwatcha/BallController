// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/*
 * SB_LOG: UE_LOG と同じ引数で、先頭に [クラス名::関数名:行番号] を付けて出力するマクロ。
 *
 * 使い方:
 *   #include "BallLog.h"
 *   SB_LOG(LogTemp, Warning, TEXT("Speed = %f"), Speed);
 *   → [ASteelBallController::Move:91] Speed = 0.500000
 *
 * Format は UE_LOG と同じく TEXT("...") の文字列リテラルで渡すこと
 * （先頭の "[%s:%d] " とリテラル同士で連結するため）。
 */

// Clang (Mac / Rider / Xcode) の __FUNCTION__ はクラス名を含まない ("Move" だけ) ので、
// __PRETTY_FUNCTION__ ("void ASteelBallController::Move(const FInputActionValue &)") から切り出す。
// MSVC の __FUNCTION__ は最初から "ASteelBallController::Move" 形式。
#if defined(__clang__) || defined(__GNUC__)
	#define SB_LOG_FUNC_SIG __PRETTY_FUNCTION__
#else
	#define SB_LOG_FUNC_SIG __FUNCTION__
#endif

namespace BallLog
{
	// "void AFoo::Bar(int)" → "AFoo::Bar"
	inline FString ShortFunctionName(const ANSICHAR* Signature)
	{
		FString Name(ANSI_TO_TCHAR(Signature));

		int32 ParenIndex = INDEX_NONE;
		if (Name.FindChar(TEXT('('), ParenIndex))
		{
			Name.LeftInline(ParenIndex);
		}

		int32 SpaceIndex = INDEX_NONE;
		if (Name.FindLastChar(TEXT(' '), SpaceIndex))
		{
			Name.RightChopInline(SpaceIndex + 1);
		}

		return Name;
	}
}

#define SB_LOG(CategoryName, Verbosity, Format, ...) \
	UE_LOG(CategoryName, Verbosity, TEXT("[%s:%d] ") Format, *BallLog::ShortFunctionName(SB_LOG_FUNC_SIG), __LINE__, ##__VA_ARGS__)
