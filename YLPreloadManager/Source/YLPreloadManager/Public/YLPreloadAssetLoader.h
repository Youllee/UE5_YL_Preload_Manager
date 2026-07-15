#pragma once

#include "CoreMinimal.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/WeakObjectPtr.h"

class UObject;
struct FStreamableHandle;
struct FYLPreloadDataBase;
enum class EPreloadContext : uint8;

struct YLPRELOADMANAGER_API FYLPreloadAssetLoadParams
{
	EPreloadContext Context;
	TWeakObjectPtr<UObject> Owner;
	TArray<TStrongObjectPtr<UObject>>* PreloadedAssets = nullptr;
	TArray<TSharedPtr<FStreamableHandle>>* AsyncLoadHandles = nullptr;
	TFunction<void(UObject*)> OnAssetStored;
};

class YLPRELOADMANAGER_API FYLPreloadAssetLoader
{
public:
	static void PreloadConfiguredAssets(const FYLPreloadAssetLoadParams& Params);
	static const TCHAR* ContextToString(EPreloadContext Context);

private:
	static bool ShouldPreloadForContext(const FYLPreloadDataBase& InPreloadData, EPreloadContext Context);
};
