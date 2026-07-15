#pragma once

#include "Modules/ModuleManager.h"

class FYLPreloadManagerModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
