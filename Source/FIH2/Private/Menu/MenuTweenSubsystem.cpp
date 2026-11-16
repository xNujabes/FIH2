#include "Menu/MenuTweenSubsystem.h"

#include "Components/Widget.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Slate/WidgetTransform.h"

namespace
{
	/** Pas de temps maximal pris en compte : évite qu'un à-coup (chargement) fasse sauter une animation. */
	const float MaxTweenDeltaTime = 0.1f;
	/** Constantes de la courbe « back » (dépassement de 10 %). */
	const float BackOvershoot = 1.70158f;
}

UMenuTweenSubsystem* UMenuTweenSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UMenuTweenSubsystem>() : nullptr;
}

void UMenuTweenSubsystem::Deinitialize()
{
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}
	Active.Empty();
	Super::Deinitialize();
}

float UMenuTweenSubsystem::ApplyEase(EMenuEase Ease, float Alpha)
{
	const float T = FMath::Clamp(Alpha, 0.f, 1.f);
	switch (Ease)
	{
	case EMenuEase::InCubic:
		return T * T * T;
	case EMenuEase::OutCubic:
		return 1.f - FMath::Pow(1.f - T, 3.f);
	case EMenuEase::InOutCubic:
		return T < 0.5f ? 4.f * T * T * T : 1.f - FMath::Pow(-2.f * T + 2.f, 3.f) / 2.f;
	case EMenuEase::InOutSine:
		return -(FMath::Cos(PI * T) - 1.f) / 2.f;
	case EMenuEase::OutBack:
		return 1.f + (BackOvershoot + 1.f) * FMath::Pow(T - 1.f, 3.f) + BackOvershoot * FMath::Pow(T - 1.f, 2.f);
	default:
		return T;
	}
}

void UMenuTweenSubsystem::Play(UWidget* Target, const FMenuTweenSpec& Spec, FName Channel)
{
	if (!Target)
	{
		return;
	}
	Stop(Target, Channel.IsNone() ? FName(TEXT("Default")) : Channel);

	FActiveTween Tween;
	Tween.Target = Target;
	Tween.Channel = Channel.IsNone() ? FName(TEXT("Default")) : Channel;
	Tween.Spec = Spec;

	// Les valeurs de départ explicites sont posées tout de suite : pendant le délai d'une cascade,
	// l'élément est déjà dans son état initial (invisible, décalé...).
	if (Spec.FromOpacity.IsSet())
	{
		Target->SetRenderOpacity(Spec.FromOpacity.GetValue());
	}
	if (Spec.FromTranslation.IsSet() || Spec.FromScale.IsSet() || Spec.FromAngle.IsSet())
	{
		FWidgetTransform Transform = Target->GetRenderTransform();
		if (Spec.FromTranslation.IsSet()) { Transform.Translation = Spec.FromTranslation.GetValue(); }
		if (Spec.FromScale.IsSet()) { Transform.Scale = Spec.FromScale.GetValue(); }
		if (Spec.FromAngle.IsSet()) { Transform.Angle = Spec.FromAngle.GetValue(); }
		Target->SetRenderTransform(Transform);
	}

	Active.Add(MoveTemp(Tween));

	if (!TickerHandle.IsValid())
	{
		TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UMenuTweenSubsystem::TickTweens));
	}
}

void UMenuTweenSubsystem::Stop(UWidget* Target, FName Channel)
{
	Active.RemoveAll([Target, Channel](const FActiveTween& Tween)
	{
		return Tween.Target.Get() == Target && (Channel.IsNone() || Tween.Channel == Channel);
	});
}

void UMenuTweenSubsystem::ResolveStart(FActiveTween& Tween, const UWidget& Widget)
{
	const FWidgetTransform Transform = Widget.GetRenderTransform();
	Tween.StartOpacity = Tween.Spec.FromOpacity.Get(Widget.GetRenderOpacity());
	Tween.StartTranslation = Tween.Spec.FromTranslation.Get(Transform.Translation);
	Tween.StartScale = Tween.Spec.FromScale.Get(Transform.Scale);
	Tween.StartAngle = Tween.Spec.FromAngle.Get(Transform.Angle);
}

void UMenuTweenSubsystem::ApplyTween(const FActiveTween& Tween, UWidget& Widget, float EasedAlpha)
{
	const FMenuTweenSpec& Spec = Tween.Spec;
	if (Spec.ToOpacity.IsSet())
	{
		Widget.SetRenderOpacity(FMath::Lerp(Tween.StartOpacity, Spec.ToOpacity.GetValue(), EasedAlpha));
	}
	if (Spec.ToTranslation.IsSet() || Spec.ToScale.IsSet() || Spec.ToAngle.IsSet())
	{
		// On ne touche qu'aux composantes animées : deux canaux peuvent animer le même widget.
		FWidgetTransform Transform = Widget.GetRenderTransform();
		if (Spec.ToTranslation.IsSet())
		{
			Transform.Translation = FMath::Lerp(Tween.StartTranslation, Spec.ToTranslation.GetValue(), EasedAlpha);
		}
		if (Spec.ToScale.IsSet())
		{
			Transform.Scale = FMath::Lerp(Tween.StartScale, Spec.ToScale.GetValue(), EasedAlpha);
		}
		if (Spec.ToAngle.IsSet())
		{
			Transform.Angle = FMath::Lerp(Tween.StartAngle, Spec.ToAngle.GetValue(), EasedAlpha);
		}
		Widget.SetRenderTransform(Transform);
	}
}

bool UMenuTweenSubsystem::TickTweens(float DeltaTime)
{
	const float Dt = FMath::Min(DeltaTime, MaxTweenDeltaTime);
	TArray<TFunction<void()>> FinishedCallbacks;

	for (int32 Index = Active.Num() - 1; Index >= 0; --Index)
	{
		FActiveTween& Tween = Active[Index];
		UWidget* Widget = Tween.Target.Get();
		if (!Widget)
		{
			Active.RemoveAtSwap(Index);
			continue;
		}

		Tween.Elapsed += Dt;
		if (Tween.Elapsed < Tween.Spec.Delay)
		{
			continue;
		}
		if (!Tween.bStarted)
		{
			Tween.bStarted = true;
			ResolveStart(Tween, *Widget);
		}

		float Alpha = Tween.Spec.Duration > 0.f ? (Tween.Elapsed - Tween.Spec.Delay) / Tween.Spec.Duration : 1.f;
		bool bDone = false;
		if (Tween.Spec.bPingPongLoop)
		{
			const float Cycle = FMath::Fmod(Alpha, 2.f);
			Alpha = Cycle <= 1.f ? Cycle : 2.f - Cycle;
		}
		else if (Alpha >= 1.f)
		{
			Alpha = 1.f;
			bDone = true;
		}

		ApplyTween(Tween, *Widget, ApplyEase(Tween.Spec.Ease, Alpha));

		if (bDone)
		{
			if (Tween.Spec.OnFinished)
			{
				FinishedCallbacks.Add(MoveTemp(Tween.Spec.OnFinished));
			}
			Active.RemoveAtSwap(Index);
		}
	}

	// Les rappels peuvent relancer des tweens : on les appelle une fois la liste stable.
	for (const TFunction<void()>& Callback : FinishedCallbacks)
	{
		Callback();
	}

	if (Active.Num() == 0)
	{
		// Plus rien à animer : le ticker se retire (retour à zéro coût au repos).
		TickerHandle.Reset();
		return false;
	}
	return true;
}
