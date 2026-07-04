// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Components/ActorComponent.h"
#include "AC_Combat.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UAC_Combat : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAC_Combat();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category = "Combat|Actions")
	void StopFire();

protected:
	virtual void BeginPlay() override;

	void PerformHit();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stats")
	float BaseDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stats")
	float FireRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Stats")
	float FireRate;
	
	FTimerHandle FireTimerHandle;
};
