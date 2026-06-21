// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "InputActionValue.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UAC_BaseMovement.generated.h"

UENUM(BlueprintType)
enum class EMovementState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Walking UMETA(DisplayName = "Walking"),
	Running UMETA(DisplayName = "Running"),
	InAir UMETA(DisplayName = "In Air"),
	Crouching UMETA(DisplayName = "Crouching"),
	Sliding UMETA(DisplayName = "Sliding"),
	Grounding UMETA(DisplayName = "Grounding"),
	WallRide UMETA(DisplayName = "WallRide"),
	Grapple UMETA(DisplayName = "Grapple"),
	Aiming UMETA(DisplayName = "Aiming")
};

UCLASS(ClassGroup=(Movement), meta=(BlueprintSpawnableComponent))
class UAC_BaseMovement : public UPawnMovementComponent
{
	GENERATED_BODY()

public:
	UAC_BaseMovement();

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Movement")
	EMovementState GetCurrentMovementState() const { return CurrentMovementState; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Movement")
	FVector GetVelocity() const { return Velocity; }

	UFUNCTION(BlueprintCallable, Category = "Movement|Input")
	void Move(FVector2D MovementVector);

	UFUNCTION(BlueprintCallable, Category = "Movement|Input")
	void Look(FVector2D LookVector);

	UFUNCTION(BlueprintCallable, Category= "Movement|Input")
	void TriggerJump();

	UFUNCTION(BlueprintCallable, Category = "Movement|Input")
	void StartCrouchSlide();

	UFUNCTION(BlueprintCallable, Category = "Movement|Input")
	void StopCrouchSlide();

	UFUNCTION(BlueprintCallable, Category = "Movement|Input")
	void TriggerDash(FVector2D DashInput);

	virtual void BeginPlay() override;

protected:
	EMovementState CurrentMovementState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	float CameraSensitivity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Walking")
	bool bIsGrounded;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Walking")
	float WalkSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Movement|Air")
	float JumpForce;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Air")
	float JumpCheckDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideFriction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideStopSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Walking")
	float CrouchSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Walking")
	float StandHalfHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Walking")
	float CrouchHalfHeight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bWantsToCrouch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Movement|Air")
	float AirSteerSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Movement|Air")
	float AirBrakeFriction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Walking")
	float GroundDeceleration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideGravityForce;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DashForce;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DashCooldown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DashVerticalInfluence;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float GroundDashMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|WallRide")
	float WallRideMinSpeed;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|WallRide")
	float WallRideFriction;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|WallRide")
	float WallRideJumpForce;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|WallRide")
	float WallRideJumpUpForce;

	UPROPERTY()
	UCapsuleComponent* PlayerCapsule;

	UPROPERTY()
	UStaticMeshComponent* PlayerVisualMesh;

	FVector CurrentFloorNormal;
	FVector CurrentWallNormal;
	float LastDashTime;
	bool bHasDashedInAir;

	bool CanStand();
	void ResizeCapsule(float NewHalfHeight);
	void EnterState(EMovementState NewState);
	void UpdateState(float DeltaTime);
	void ExitState(EMovementState OldState);

};
