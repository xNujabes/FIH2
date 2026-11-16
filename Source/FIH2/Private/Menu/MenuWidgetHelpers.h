// Petites fabriques partagées par les widgets du menu (arbres UMG construits en C++, comme UTutorialPanelWidget).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "UObject/UnrealType.h"

namespace FIH2Menu
{
	/** Brosse « boîte arrondie » avec contour optionnel. */
	inline FSlateBrush RoundedBrush(const FLinearColor& Fill, float Radius,
		const FLinearColor& Outline = FLinearColor::Transparent, float OutlineWidth = 0.f)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Fill);
		Brush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
		Brush.OutlineSettings.Color = FSlateColor(Outline);
		Brush.OutlineSettings.Width = OutlineWidth;
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		return Brush;
	}

	/** Style de bouton à partir de quatre brosses, sans décalage de contenu à la pression. */
	inline FButtonStyle MakeButtonStyle(const FSlateBrush& Normal, const FSlateBrush& Hovered,
		const FSlateBrush& Pressed, const FSlateBrush& Disabled)
	{
		FButtonStyle Style;
		Style.SetNormal(Normal);
		Style.SetHovered(Hovered);
		Style.SetPressed(Pressed);
		Style.SetDisabled(Disabled);
		Style.SetNormalPadding(FMargin(0.f));
		Style.SetPressedPadding(FMargin(0.f));
		return Style;
	}

	inline void SetFont(UTextBlock* Text, int32 Size, bool bBold = false)
	{
		if (!Text)
		{
			return;
		}
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Font.TypefaceFontName = bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular"));
		Text->SetFont(Font);
	}

	inline UTextBlock* MakeText(UWidgetTree* Tree, const FText& Text, int32 Size, const FLinearColor& Color,
		bool bBold = false, FName Name = NAME_None)
	{
		UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Block->SetText(Text);
		Block->SetColorAndOpacity(FSlateColor(Color));
		SetFont(Block, Size, bBold);
		return Block;
	}

	/**
	 * Fixe une propriété booléenne par réflexion. Sert à « IsFocusable » des widgets UMG, qui ne se règle
	 * qu'avant la construction du widget Slate et dont l'accès direct en C++ varie selon les versions du moteur.
	 */
	inline void SetBoolProperty(UObject* Object, FName PropertyName, bool bValue)
	{
		if (!Object)
		{
			return;
		}
		if (const FBoolProperty* Property = FindFProperty<FBoolProperty>(Object->GetClass(), PropertyName))
		{
			Property->SetPropertyValue_InContainer(Object, bValue);
		}
	}
}
