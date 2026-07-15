#include "YLPreloadWorldSubsystem.h"

#include "YLPreloadAssetLoader.h"
#include "YLPreloadManagerType.h"

#include "Engine/World.h"

bool UYLPreloadWorldSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (Super::ShouldCreateSubsystem(Outer) == false)
	{
		return false;
	}

	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

void UYLPreloadWorldSubsystem::PostInitialize()
{
	Super::PostInitialize();

	PreloadForWorld();
}

void UYLPreloadWorldSubsystem::Deinitialize()
{
	ResetPreloadedAssets();

	Super::Deinitialize();
}

void UYLPreloadWorldSubsystem::PreloadForWorld()
{
	ResetPreloadedAssets();

	for (const EPreloadContext Context : GetPreloadContextsForWorld())
	{
		FYLPreloadAssetLoadParams Params;
		Params.Context = Context;
		Params.Owner = this;
		Params.PreloadedAssets = &PreloadedAssets;
		Params.AsyncLoadHandles = &AsyncLoadHandles;

		FYLPreloadAssetLoader::PreloadConfiguredAssets(Params);
	}
}

void UYLPreloadWorldSubsystem::ResetPreloadedAssets()
{
	AsyncLoadHandles.Reset();
	PreloadedAssets.Reset();
}

TArray<EPreloadContext> UYLPreloadWorldSubsystem::GetPreloadContextsForWorld() const
{
	TArray<EPreloadContext> Contexts;

	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return Contexts;
	}

	switch (World->GetNetMode())
	{
	case NM_DedicatedServer:
		Contexts.Add(EPreloadContext::GameServer);
		break;
	case NM_Client:
		Contexts.Add(EPreloadContext::GameClient);
		break;
	case NM_ListenServer:
	case NM_Standalone:
		Contexts.Add(EPreloadContext::GameServer);
		Contexts.Add(EPreloadContext::GameClient);
		break;
	default:
		break;
	}

	return Contexts;
}
