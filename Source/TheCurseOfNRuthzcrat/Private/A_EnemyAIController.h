#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "A_EnemyAIController.generated.h"

UCLASS()
class AA_EnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AA_EnemyAIController();

protected:
	virtual void BeginPlay() override;

	// Le composant qui donne des "yeux" à l'IA
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	class UAIPerceptionComponent* AIPerception;

	class UAISenseConfig_Sight* SightConfig;

	// Fonction appelée quand l'IA voit (ou perd de vue) un acteur
	UFUNCTION()
	void OnTargetDetected(AActor* Actor, FAIStimulus const Stimulus);
};