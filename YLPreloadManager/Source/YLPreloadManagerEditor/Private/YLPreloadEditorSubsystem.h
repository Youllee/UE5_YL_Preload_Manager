#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "YLPreloadEditorSubsystem.generated.h"

class UPackage;
class UYLPreloadManagerSettings;
class FObjectPostSaveContext;
struct FPropertyChangedEvent;
struct FStreamableHandle;

UCLASS()
class UYLPreloadEditorSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void RegisterDelegates();
	void UnregisterDelegates();
	void RequestPreloadForEditor();
	void PreloadForEditor();
	void ResetPreloadedAssets();
	void OnRefreshPreloadAssets();
	void OnPreloadAssetSaved(const FString& InPackageFileName, UPackage* InPackage, FObjectPostSaveContext InObjectSaveContext);

private:
	bool bIsRefreshingPreload = false;

	TArray<TStrongObjectPtr<UObject>> PreloadedAssets;
	TArray<TSharedPtr<FStreamableHandle>> AsyncLoadHandles;
	TArray<TWeakObjectPtr<UObject>> WeakPreloadAssets;
	TWeakObjectPtr<UYLPreloadManagerSettings> WeakPreloadSettings;

	FDelegateHandle PreloadSettingsChangedHandle;
	FDelegateHandle PreloadAssetSavedHandle;
	FDelegateHandle PostEngineInitHandle;
};
