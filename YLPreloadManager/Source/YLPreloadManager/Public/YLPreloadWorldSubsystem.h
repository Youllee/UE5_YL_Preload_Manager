#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "YLPreloadWorldSubsystem.generated.h"

struct FStreamableHandle;
enum class EPreloadContext : uint8;

UCLASS()
class YLPRELOADMANAGER_API UYLPreloadWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void PostInitialize() override;
	virtual void Deinitialize() override;

private:
	void PreloadForWorld();
	void ResetPreloadedAssets();
	TArray<EPreloadContext> GetPreloadContextsForWorld() const;

private:
	TArray<TStrongObjectPtr<UObject>> PreloadedAssets;
	TArray<TSharedPtr<FStreamableHandle>> AsyncLoadHandles;
};
