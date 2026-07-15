#include "YLPreloadAssetLoader.h"

#include "YLPreloadManagerLog.h"
#include "YLPreloadManagerSettings.h"
#include "YLPreloadManagerType.h"

#include "DataRegistrySubsystem.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"

void FYLPreloadAssetLoader::PreloadConfiguredAssets(const FYLPreloadAssetLoadParams& Params)
{
	if (Params.Owner.IsValid() == false || Params.PreloadedAssets == nullptr || Params.AsyncLoadHandles == nullptr)
	{
		return;
	}

	const UYLPreloadManagerSettings* Settings = GetDefault<UYLPreloadManagerSettings>();
	if (Settings == nullptr)
	{
		return;
	}

	TWeakObjectPtr<UObject> WeakOwner = Params.Owner;
	TArray<TStrongObjectPtr<UObject>>* TargetPreloadedAssets = Params.PreloadedAssets;
	TArray<TSharedPtr<FStreamableHandle>>* TargetAsyncLoadHandles = Params.AsyncLoadHandles;
	TFunction<void(UObject*)> OnAssetStored = Params.OnAssetStored;
	const EPreloadContext Context = Params.Context;

	auto StoreLoadedObject = [WeakOwner, TargetPreloadedAssets, OnAssetStored](UObject* LoadedObject)
		{
			if (WeakOwner.IsValid() == false || LoadedObject == nullptr || TargetPreloadedAssets == nullptr)
			{
				return;
			}

			TargetPreloadedAssets->Add(TStrongObjectPtr<UObject>(LoadedObject));

			if (OnAssetStored)
			{
				OnAssetStored(LoadedObject);
			}
		};

	auto PreloadSoftObject = [WeakOwner, Context, TargetAsyncLoadHandles, StoreLoadedObject](const FYLPreloadDataBase& PreloadData, const FSoftObjectPath& ObjectPath, TFunction<void(UObject*)> OnLoaded)
		{
			if (WeakOwner.IsValid() == false)
			{
				return;
			}

			if (ShouldPreloadForContext(PreloadData, Context) == false)
			{
				return;
			}

			if (ObjectPath.IsNull())
			{
				UE_LOG(LogYLPreloadManager, Warning, TEXT("Preload failed. Asset=None, LoadType=%s, Context=%s, Reason=SoftObjectPtr is null"),
					PreloadData.bUseAsyncLoad ? TEXT("Async") : TEXT("Sync"),
					ContextToString(Context));
				return;
			}

			// 이미 메모리에 올라와 있는 에셋은 다시 로드 요청을 보내지 않는다.
			// 단, 프리로드 수명은 소유 서브시스템이 보장해야 하므로 strong reference 배열에는 다시 보관한다.
			if (UObject* LoadedObject = ObjectPath.ResolveObject())
			{
				StoreLoadedObject(LoadedObject);

				UE_LOG(LogYLPreloadManager, Log, TEXT("Preload skipped. Asset=%s, LoadType=%s, Context=%s, Reason=Asset is already loaded"),
					*ObjectPath.ToString(),
					PreloadData.bUseAsyncLoad ? TEXT("Async") : TEXT("Sync"),
					ContextToString(Context));
				return;
			}

			if (PreloadData.bUseAsyncLoad)
			{
				UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
				if (AssetManager == nullptr)
				{
					UE_LOG(LogYLPreloadManager, Warning, TEXT("Preload failed. Asset=%s, LoadType=Async, Context=%s, Reason=AssetManager is not initialized"),
						*ObjectPath.ToString(),
						ContextToString(Context));
					return;
				}

				TSharedPtr<FStreamableHandle> AsyncLoadHandle = AssetManager->GetStreamableManager().RequestAsyncLoad(
					ObjectPath,
					FStreamableDelegate::CreateLambda(
						[WeakOwner, Context, ObjectPath, StoreLoadedObject, OnLoaded]() mutable
						{
							if (WeakOwner.IsValid() == false)
							{
								return;
							}

							if (UObject* LoadedObject = ObjectPath.ResolveObject())
							{
								StoreLoadedObject(LoadedObject);

								if (OnLoaded)
								{
									OnLoaded(LoadedObject);
								}

								UE_LOG(LogYLPreloadManager, Log, TEXT("Preload completed. Asset=%s, LoadType=Async, Context=%s"),
									*ObjectPath.ToString(),
									ContextToString(Context));
							}
							else
							{
								UE_LOG(LogYLPreloadManager, Warning, TEXT("Preload failed. Asset=%s, LoadType=Async, Context=%s, Reason=Loaded object could not be resolved after async request completed"),
									*ObjectPath.ToString(),
									ContextToString(Context));
							}
						}));

				if (AsyncLoadHandle.IsValid())
				{
					TargetAsyncLoadHandles->Add(AsyncLoadHandle);
				}
				else
				{
					UE_LOG(LogYLPreloadManager, Warning, TEXT("Preload failed. Asset=%s, LoadType=Async, Context=%s, Reason=RequestAsyncLoad returned an invalid handle"),
						*ObjectPath.ToString(),
						ContextToString(Context));
				}
			}
			else
			{
				if (UObject* LoadedObject = ObjectPath.TryLoad())
				{
					StoreLoadedObject(LoadedObject);

					if (OnLoaded)
					{
						OnLoaded(LoadedObject);
					}

					UE_LOG(LogYLPreloadManager, Log, TEXT("Preload completed. Asset=%s, LoadType=Sync, Context=%s"),
						*ObjectPath.ToString(),
						ContextToString(Context));
				}
				else
				{
					UE_LOG(LogYLPreloadManager, Warning, TEXT("Preload failed. Asset=%s, LoadType=Sync, Context=%s, Reason=LoadSynchronous returned null"),
						*ObjectPath.ToString(),
						ContextToString(Context));
				}
			}
		};

	for (const FYLDataAssetPreloadData& DataAssetData : Settings->DataAssets)
	{
		PreloadSoftObject(DataAssetData, DataAssetData.DataAsset.ToSoftObjectPath(), TFunction<void(UObject*)>());
	}

	for (const FYLDataTablePreloadData& DataTableData : Settings->DataTables)
	{
		PreloadSoftObject(DataTableData, DataTableData.DataTable.ToSoftObjectPath(), TFunction<void(UObject*)>());
	}

	if (UDataRegistrySubsystem::Get())
	{
		for (const FYLDataRegistryPreloadData& DataRegistryData : Settings->DataRegistries)
		{
			const FSoftObjectPath DataRegistryPath = DataRegistryData.DataRegistry.ToSoftObjectPath();
			PreloadSoftObject(
				DataRegistryData,
				DataRegistryPath,
				[DataRegistryPath](UObject* LoadedObject)
				{
					if (UDataRegistrySubsystem* LoadedDataRegistrySubsystem = UDataRegistrySubsystem::Get())
					{
						LoadedDataRegistrySubsystem->LoadRegistryPath(DataRegistryPath);
					}
					else
					{
						UE_LOG(LogYLPreloadManager, Warning, TEXT("Preload post-process failed. Asset=%s, Reason=DataRegistrySubsystem is not available"),
							*DataRegistryPath.ToString());
					}

					if (UDataRegistry* DataRegistry = Cast<UDataRegistry>(LoadedObject))
					{
						if (DataRegistry->IsInitialized())
						{
							DataRegistry->ResetRuntimeState();
						}
						else
						{
							UE_LOG(LogYLPreloadManager, Log, TEXT("Preload post-process skipped. Asset=%s, Reason=DataRegistry is not initialized"),
								*DataRegistryPath.ToString());
						}
					}
					else
					{
						UE_LOG(LogYLPreloadManager, Warning, TEXT("Preload post-process failed. Asset=%s, Reason=Loaded object is not a DataRegistry"),
							*DataRegistryPath.ToString());
					}
				});
		}
	}
}

const TCHAR* FYLPreloadAssetLoader::ContextToString(EPreloadContext Context)
{
	switch (Context)
	{
	case EPreloadContext::EditorWorld:	return TEXT("EditorWorld");
	case EPreloadContext::GameServer:	return TEXT("GameServer");
	case EPreloadContext::GameClient:	return TEXT("GameClient");
	}

	return TEXT("Unknown");
}

bool FYLPreloadAssetLoader::ShouldPreloadForContext(const FYLPreloadDataBase& InPreloadData, EPreloadContext Context)
{
	switch (Context)
	{
	case EPreloadContext::EditorWorld:	return InPreloadData.bPreloadOnEditor;
	case EPreloadContext::GameServer:	return InPreloadData.bPreloadOnServer;
	case EPreloadContext::GameClient:	return InPreloadData.bPreloadOnClient;
	}

	return false;
}
