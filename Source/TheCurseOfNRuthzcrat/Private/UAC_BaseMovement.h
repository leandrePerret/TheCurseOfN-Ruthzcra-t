// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "InputActionValue.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "CableComponent.h"
#include "UAC_BaseMovement.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGrappleStartedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGrappleStoppedDelegate);

UENUM(BlueprintType)
enum class EMovementState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Running UMETA(DisplayName = "Running"),
	InAir UMETA(DisplayName = "In Air"),
	Crouching UMETA(DisplayName = "Crouching"),
	Sliding UMETA(DisplayName = "Sliding"),
	Grounding UMETA(DisplayName = "Grounding"),
	WallRide UMETA(DisplayName = "WallRide"),
	Grapple UMETA(DisplayName = "Grapple"),
	Aiming UMETA(DisplayName = "Aiming"),
	Dashing UMETA(DisplayName= "Dashing")
};

UCLASS(ClassGroup=(Movement), meta=(BlueprintSpawnableComponent))
class UAC_BaseMovement : public UPawnMovementComponent
{
	GENERATED_BODY()

public:
	UAC_BaseMovement();

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category = "Movement")
	EMovementState GetCurrentMovementState() const { return CurrentMovementState; }

	UFUNCTION(BlueprintPure, Category = "Movement")
	FVector GetVelocity() const { return Velocity; }

	UFUNCTION(BlueprintPure, Category = "Movement")
	AActor* GetGrappleAnchorActor() const { return GrappleAnchorActor; }

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

	UFUNCTION(BlueprintCallable, Category = "Movement|Input")
	void StartGrapple();
	
	UFUNCTION(BlueprintCallable, Category = "Movement|Input")
	void StopGrapple();

	UPROPERTY(BlueprintAssignable, Category = "Movement|Grapple")
	FOnGrappleStartedDelegate OnGrappleStarted;

	UPROPERTY(BlueprintAssignable, Category = "Movement|Grapple")
	FOnGrappleStoppedDelegate OnGrappleStopped;

	virtual void BeginPlay() override;

protected:
	EMovementState CurrentMovementState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
	float CameraSensitivity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Running")
	bool bIsGrounded;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Running")
	float RunSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Running")
	float GroundDeceleration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Running")
	float CrouchSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Running")
	float StandHalfHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Running")
	float CrouchHalfHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Movement|Air")
	float JumpForce;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Air")
	float JumpCheckDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Movement|Air")
	float AirSteerSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Movement|Air")
	float AirBrakeFriction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideFriction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideStopSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideGravityForce;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bWantsToCrouch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|WallRide")
	float WallRideMinSpeed;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|WallRide")
	float WallRideFriction;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|WallRide")
	float WallRideJumpForce;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|WallRide")
	float WallRideJumpUpForce;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|WallRide")
	int32 MaxWallJumps;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DashSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DashDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DashBoost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DashCooldown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
	float DashVerticalInfluence;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Dash")
	bool bHasDashedInAir;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Dash")
	float LastDashTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grapple")
	float MaxGrappleDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grapple")
	float GrapplePullSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Grapple")
	FVector GrapplePoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grapple")
	float GrappleCooldown;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Grapple")
	bool bHasGrappledInAir;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Grapple")
	float LastGrappleTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grapple")
	float GrappleEndBoost;

	UPROPERTY(EditAnywhere, Category = "Movement|Grapple")
	TSubclassOf<AActor> GrappleAnchorClass;

	UPROPERTY()
	UCapsuleComponent* PlayerCapsule;

	UPROPERTY()
	USkeletalMeshComponent* PlayerVisualMesh;

	UPROPERTY()
	UCableComponent* GrappleCable;

	UPROPERTY()
	AActor* GrappleAnchorActor;

	FVector CurrentFloorNormal;
	FVector CurrentWallNormal;
	FVector DashDirection;
	FVector PreDashVelocity;
	float DashDistanceRemaining; 
	int32 WallJumpCount;

	void UpdateRotation(float DeltaTime, FVector InputVector);
	void UpdateVelocity(float DeltaTime, FVector InputVector);
	void UpdateMovement(float DeltaTime, FVector InputVector);
	void UpdateState(float DeltaTime);

	void WallRideRotation(float DeltaTime, FVector InputVector);
	void SlidingRotation(float DeltaTime, FVector InputVector);
	void DashingRotation(float DeltaTime, FVector InputVector);
	void GrappleRotation(float DeltaTime, FVector InputVector);
	void DefaultRotation(float DeltaTime, FVector InputVector);

	void WallRideVelocity(float DeltaTime, FVector InputVector);
	void SlidingVelocity(float DeltaTime, FVector InputVector);
	void DashingVelocity(float DeltaTime, FVector InputVector);
	void GrappleVelocity(float DeltaTime, FVector InputVector);
	void InAirVelocity(float DeltaTime, FVector InputVector);
	void DefaultVelocity(float DeltaTime, FVector InputVector);


	bool CanStand();
	void ResizeCapsule(float NewHalfHeight);
	void EnterState(EMovementState NewState);
};
