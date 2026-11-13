#include "System/TutorialSequence.h"

#include "Fish/Fish.h"
#include "InputAction.h"

#define LOCTEXT_NAMESPACE "FIH2Tutorial"

UTutorialSequence::UTutorialSequence()
{
	MaintainResetMessage = LOCTEXT("MaintainReset", "Équilibre rompu ! Le compte à rebours redémarre.");
	EggLostMessage = LOCTEXT("EggLost", "L'œuf est mort : la pollution était trop forte. Attends une nouvelle ponte.");

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
		FTutorialStep& Step = AddStep(LOCTEXT("S5", "Tes poissons ont faim ! Regarde la jauge de faim : elle est passée au rouge."),
			ETutorialCondition::GaugeZoneReached);
		Step.GaugeVariable = ETutorialGaugeVariable::Hunger;
		Step.Zone = EEcosystemZone::Yellow;
		// Les kelps proches nourrissent les poissons tout seuls : sans cette famine imposée, la faim ne monte jamais
		// et les poissons rassasiés refusent ensuite la nourriture du joueur (test du 13/11).
		Step.SetFishHungerOnEnter = 0;
		Step.MinDisplayTime = 4.f;
		Step.Timeout = 60.f;
	}

	// 6. Nourrir
	{
		FTutorialStep& Step = AddStep(LOCTEXT("S6", "Nourris-les : attrape un fruit sur un kelp et lâche-le près d'un poisson affamé."),
			ETutorialCondition::FishAte);
		Step.bRequirePlayerFood = true;
		Step.Timeout = 120.f; // anti-blocage
	}

	// 7. Kelps éloignés
	{
		FTutorialStep& Step = AddStep(LOCTEXT("S7", "Ces kelps sont trop loin : leurs fruits tombent sans être mangés, pourrissent et polluent l'eau. Rapproche-les de tes poissons."),
			ETutorialCondition::TaggedActorsNear);
		Step.TargetTag = TEXT("TutoKelpFar");
		Step.ReferenceTag = TEXT("TutoKelpNear");
		Step.NearRadius = 1500.f;
		Step.HighlightTag = TEXT("TutoKelpFar");
		Step.CameraFocusTag = TEXT("TutoKelpFar");
		Step.Timeout = 90.f;
	}

	// 8. Ponte (TUTO_DELAI_PONTE = 8 s [5–20]) : sans ce raccourci, la première ponte peut prendre plus de 2 minutes.
	{
		FTutorialStep& Step = AddStep(LOCTEXT("S8", "Un poisson bien nourri (faim 2/3 ou plus) pond des œufs. Garde tes poissons nourris et attends le premier œuf."),
			ETutorialCondition::EggLaid);
		Step.ShortenReproTimersTo = 8.f;
	}

	// 9. Incubation (20 s, réglage de l'espèce)
	{
		FTutorialStep& Step = AddStep(LOCTEXT("S9", "Un œuf ! Il éclot en 20 s. Si tu le tiens, il n'incube plus ; si la pollution passe au rouge, il meurt."),
			ETutorialCondition::EggHatched);
		Step.bFocusEventActor = true;
	}

	// 10. Alevin
	{
		FTutorialStep& Step = AddStep(LOCTEXT("S10", "Un alevin est né ! Clique dessus pour suivre sa croissance."),
			ETutorialCondition::SelectFry);
		Step.bFocusEventActor = true;
		Step.Timeout = 60.f;
	}

	// 11. Maintien (TUTO_DUREE_MAINTIEN = 60 s [30–120])
	{
		FTutorialStep& Step = AddStep(LOCTEXT("S8", "Bravo ! Garde l'écosystème hors de la zone rouge pendant 1 minute."),
			ETutorialCondition::Maintain);
		Step.Duration = 60.f;
	}
}

#undef LOCTEXT_NAMESPACE
