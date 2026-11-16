#include "Menu/MenuTheme.h"

#include "System/FIH2ProjectSettings.h"

#define LOCTEXT_NAMESPACE "FIH2Menu"

UMenuTheme::UMenuTheme()
{
	GameTitle = LOCTEXT("GameTitle", "FIH2");
	GameSubtitle = LOCTEXT("GameSubtitle", "Faites vivre votre récif");
}

const UMenuTheme& UMenuTheme::Get()
{
	if (const UMenuTheme* Theme = UFIH2ProjectSettings::Get()->MenuTheme.LoadSynchronous())
	{
		return *Theme;
	}
	return *GetDefault<UMenuTheme>();
}

#undef LOCTEXT_NAMESPACE
