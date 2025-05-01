// Copyright (c) Jared Taylor


#include "VigilCustomization.h"

#include "DetailLayoutBuilder.h"


TSharedRef<IDetailCustomization> FVigilCustomization::MakeInstance()
{
	return MakeShared<FVigilCustomization>();
}

void FVigilCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DetailBuilder.EditCategory(TEXT("Vigil"), FText::GetEmpty(), ECategoryPriority::Important);
}