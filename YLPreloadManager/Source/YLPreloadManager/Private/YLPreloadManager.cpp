// Copyright Epic Games, Inc. All Rights Reserved.

#include "YLPreloadManager.h"

#include "YLPreloadManagerLog.h"

#define LOCTEXT_NAMESPACE "FYLPreloadManagerModule"

DEFINE_LOG_CATEGORY(LogYLPreloadManager);

void FYLPreloadManagerModule::StartupModule()
{
}

void FYLPreloadManagerModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FYLPreloadManagerModule, YLPreloadManager)
