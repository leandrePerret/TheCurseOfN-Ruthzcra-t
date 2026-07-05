#include "A_EnemyAIController.h"

AA_EnemyAIController::AA_EnemyAIController()
{
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

	SightConfig->SightRadius = 1500.f; // Distance de vision
	SightConfig->LoseSightRadius = 2000.f; // Distance où elle te perd
	SightConfig->PeripheralVisionAngleDegrees = 60.f; // Champ de vision (120° total)
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	AIPerception->ConfigureSense(*SightConfig);
	AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());
}

void AA_EnemyAIController::BeginPlay()
{
	Super::BeginPlay();
	AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &AA_EnemyAIController::OnTargetDetected);
}

void AA_EnemyAIController::OnTargetDetected(AActor* Actor, FAIStimulus const Stimulus)
{
	// Si c'est le joueur (tu peux faire un Cast vers ta classe de joueur pour être sûr)
	if (Actor->ActorHasTag("Player"))
	{
		// Si le Blackboard existe, on lui dit si on voit le joueur ou non
		if (GetBlackboardComponent())
		{
			// Stimulus.WasSuccessfullySensed() est True si on le voit, False si on le perd de vue
			GetBlackboardComponent()->SetValueAsObject(FName("TargetPlayer"), Stimulus.WasSuccessfullySensed() ? Actor : nullptr);
		}
	}
}