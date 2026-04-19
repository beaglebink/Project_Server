#pragma once

#include "Modules/ModuleManager.h"

class FLocationEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};