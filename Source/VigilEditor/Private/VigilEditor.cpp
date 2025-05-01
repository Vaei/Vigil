// Copyright (c) Jared Taylor

#include "VigilEditor.h"

#include "VigilComponent.h"
#include "VigilCustomization.h"

#define LOCTEXT_NAMESPACE "FVigilEditorModule"

void FVigilEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	
	PropertyModule.RegisterCustomClassLayout(UVigilComponent::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FVigilCustomization::MakeInstance));
}

void FVigilEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule* PropertyModule = FModuleManager::Get().GetModulePtr<FPropertyEditorModule>("PropertyEditor");
		PropertyModule->UnregisterCustomClassLayout(UVigilComponent::StaticClass()->GetFName());
	}
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FVigilEditorModule, VigilEditor)