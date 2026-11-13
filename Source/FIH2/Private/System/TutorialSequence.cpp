#include "System/TutorialSequence.h"

#include "Fish/Fish.h"
#include "InputAction.h"

#define LOCTEXT_NAMESPACE "FIH2Tutorial"

UTutorialSequence::UTutorialSequence()
{
	MaintainResetMessage = LOCTEXT("MaintainReset", "Équilibre rompu ! Le compte à rebours redémarre.");

	auto AddStep = [this](const FText& Message, ETutorialCondition Condition) -> FTutorialStep&
	{
		FTutorialStep& Step = Steps.AddDefaulted_GetRef();
		Step.Message = Message;
		Step.Condition = Condition;
		return Step;
	};

	// 0. Accueil
	AddStep(LOCTEXT("S0", "Bienvenue dans ta cuve ! Ton rôle : garder cet écosystème en équilibre."),
		ETutorialCondition::NextButton);

	// 1. Caméra (IA_Look n'est pas utilisé : il se déclenche à chaque mouvement de souris)
	{
		FTutorialStep& Step = AddStep(LOCTEXT("S1", "Déplace la caméra pour explorer la cuve (déplacement, rotation, molette pour zoomer)."),
			ETutorialCondition::InputAction);
		Step.InputActions.Add(TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/System/IA_Move.IA_Move"))));
		Step.InputActions.Add(TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/System/IA_EnableFreeCam.IA_EnableFreeCam"))));
		Step.InputActions.Add(TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/System/IA_Zoom.IA_Zoom"))));
		Step.MinDisplayTime = 2.f;
	}

	// 2. Sélection
	{
		FTutorialStep& Step = AddStep(LOCTEXT("S2", "Clique sur un poisson pour afficher ses informations."),
			ETutorialCondition::SelectActorOfClass);
		Step.TargetClass = AFish::StaticClass();
	}

	// 3. Loupe
	AddStep(LOCTEXT("S3", "Clique sur la loupe du panneau pour observer le poisson de près."),
		ETutorialCondition::FocusUsed);

	// 4. Observation (TUTO_DELAI_OBSERVATION = 15 s [10–30])
	{
		FTutorialStep& Step = AddStep(LOCTEXT("S4", "Observe tes poissons. Leur faim baisse avec le temps : surveille les jauges de la cuve."),
			ETutorialCondition::Delay);
		Step.Duration = 15.f;
	}

	// 5. Jauge de faim
	{
		FTutorialStep& Step = AddStep(LOCTEXT("S5", "La jauge de faim monte ! Certains poissons sont affamés."),
			ETutorialCondition::GaugeZoneReached);
		Step.GaugeVariable = ETutorialGaugeVariable::Hunger;
		Step.Zone = EEcosystemZone::Yellow;
		Step.Timeout = 60.f; // les poissons mangent seuls les fruits des kelps proches : la faim peut ne jamais monter
	}

	// 6. Nourrir
	{
		FTutorialStep& Step = AddStep(LOCTEXT("S6", "Nourris-les : attrape un fruit sur un kelp et lâche-le près d'un poisson affamé."),
			ETutorialCondition::FishAte);
		Step.bRequirePlayerFood = true;
	}

	// 7. Kelps éloignés
	{
		FTutorialStep& Step = AddStep(LOCTEXT("S7", "Ces kelps sont trop loin : leurs fruits tombent sans être mangés, pourrissent et polluent l'eau. Rapproche-les de tes poissons."),
			ETutorialCondition::TaggedActorsNear);
		Step.TargetTag = TEXT("TutoKelpFar");
		Step.ReferenceTag = TEXT("TutoKelpNear");
		Step.NearRadius = 1500.f;
		Step.HighlightTag = TEXT("TutoKelpFar");
		Step.Timeout = 90.f;
	}

	// 8. Maintien (TUTO_DUREE_MAINTIEN = 60 s [30–120])
	{
		FTutorialStep& Step = AddStep(LOCTEXT("S8", "Bravo ! Garde l'écosystème hors de la zone rouge pendant 1 minute."),
			ETutorialCondition::Maintain);
		Step.Duration = 60.f;
	}
}

#undef LOCTEXT_NAMESPACE
