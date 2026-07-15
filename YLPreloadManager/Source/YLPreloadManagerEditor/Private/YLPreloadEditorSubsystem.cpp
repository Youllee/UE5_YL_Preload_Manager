#include "YLPreloadEditorSubsystem.h"

#include "YLPreloadAssetLoader.h"
#include "YLPreloadManagerSettings.h"
#include "YLPreloadManagerType.h"

#include "Engine/AssetManager.h"
#include "Misc/CoreDelegates.h"
#include "Templates/UnrealTemplate.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/UObjectGlobals.h"

void UYLPreloadEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	RegisterDelegates();
	RequestPreloadForEditor();
}

void UYLPreloadEditorSubsystem::Deinitialize()
{
	UnregisterDelegates();
	ResetPreloadedAssets();

	Super::Deinitialize();
}

void UYLPreloadEditorSubsystem::RegisterDelegates()
{
	if (UYLPreloadManagerSettings* PreloadSettings = GetMutableDefault<UYLPreloadManagerSettings>())
	{
		WeakPreloadSettings = PreloadSettings;
		PreloadSettingsChangedHandle = PreloadSettings->OnSettingChanged().AddLambda(
			[this](UObject* SettingsObject, FPropertyChangedEvent& PropertyChangedEvent)
			{
				OnRefreshPreloadAssets();
			});
	}
	else
	{
		WeakPreloadSettings.Reset();
	}

	PreloadAssetSavedHandle = UPackage::PackageSavedWithContextEvent.AddLambda(
		[this](const FString& InPackageFileName, UPackage* InPackage, FObjectPostSaveContext InObjectSaveContext)
		{
			OnPreloadAssetSaved(InPackageFileName, InPackage, InObjectSaveContext);
		});
}

void UYLPreloadEditorSubsystem::UnregisterDelegates()
{
	if (PostEngineInitHandle.IsValid())
	{
		FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitHandle);
		PostEngineInitHandle.Reset();
	}

	if (PreloadSettingsChangedHandle.IsValid())
	{
		if (UYLPreloadManagerSettings* PreloadSettings = WeakPreloadSettings.Get())
		{
			PreloadSettings->OnSettingChanged().Remove(PreloadSettingsChangedHandle);
		}

		PreloadSettingsChangedHandle.Reset();
		WeakPreloadSettings.Reset();
	}

	if (PreloadAssetSavedHandle.IsValid())
	{
		UPackage::PackageSavedWithContextEvent.Remove(PreloadAssetSavedHandle);
		PreloadAssetSavedHandle.Reset();
	}
}

void UYLPreloadEditorSubsystem::RequestPreloadForEditor()
{
	if (UAssetManager::IsInitialized())
	{
		PreloadForEditor();
		return;
	}

	if (PostEngineInitHandle.IsValid())
	{
		return;
	}

	TWeakObjectPtr<UYLPreloadEditorSubsystem> WeakThis(this);
	PostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddLambda(
		[WeakThis]()
		{
			if (UYLPreloadEditorSubsystem* Subsystem = WeakThis.Get())
			{
				Subsystem->PostEngineInitHandle.Reset();
				Subsystem->PreloadForEditor();
			}
		});
}

void UYLPreloadEditorSubsystem::PreloadForEditor()
{
	ResetPreloadedAssets();
	WeakPreloadAssets.Reset();

	FYLPreloadAssetLoadParams Params;
	Params.Context = EPreloadContext::EditorWorld;
	Params.Owner = this;
	Params.PreloadedAssets = &PreloadedAssets;
	Params.AsyncLoadHandles = &AsyncLoadHandles;
	Params.OnAssetStored = [this](UObject* LoadedObject)
		{
			if (LoadedObject)
			{
				WeakPreloadAssets.AddUnique(LoadedObject);
			}
		};

	FYLPreloadAssetLoader::PreloadConfiguredAssets(Params);
}

void UYLPreloadEditorSubsystem::ResetPreloadedAssets()
{
	AsyncLoadHandles.Reset();
	PreloadedAssets.Reset();
}

void UYLPreloadEditorSubsystem::OnRefreshPreloadAssets()
{
	if (bIsRefreshingPreload == true)
	{
		return;
	}
	TGuardValue<bool> RefreshGuard(bIsRefreshingPreload, true);

	RequestPreloadForEditor();
}

void UYLPreloadEditorSubsystem::OnPreloadAssetSaved(const FString& /*InPackageFileName*/, UPackage* InPackage, FObjectPostSaveContext InObjectSaveContext)
{
	if (InObjectSaveContext.IsCooking() || InPackage == nullptr)
	{
		return;
	}

	TArray<UObject*> SavedObjects;
	constexpr bool bIncludeNestedObjects = false;
	GetObjectsWithPackage(InPackage, SavedObjects, bIncludeNestedObjects);
	for (UObject* SavedObject : SavedObjects)
	{
		if (SavedObject == nullptr)
		{
			continue;
		}

		for (const TWeakObjectPtr<UObject>& WatchedAsset : WeakPreloadAssets)
		{
			if (WatchedAsset.Get() == SavedObject)
			{
				OnRefreshPreloadAssets();
				return;
			}
		}
	}
}
