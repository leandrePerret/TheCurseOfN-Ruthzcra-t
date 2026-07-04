#include "UAC_BaseMovement.h"

UAC_BaseMovement::UAC_BaseMovement()
{
	// All good nothing to touch, default values are set here
	PrimaryComponentTick.bCanEverTick = true;
	CurrentMovementState = EMovementState::Idle;
	CameraSensitivity = 1.f;
	RunSpeed = 600.f;
	JumpForce = 600.f;
	JumpCheckDistance = 20.f;
	SlideMultiplier = 1.5f;
	SlideFriction = 0.5f;
	SlideStopSpeed = 200.f;
	SlideThreshold = 100.f;
	CrouchSpeed = 300.f;
	StandHalfHeight = 88.f;
	CrouchHalfHeight = 44.f;
	AirSteerSpeed = 5.f;
	AirBrakeFriction = 2.f;
	GroundDeceleration = 5.f;
	SlideGravityForce = 3500.f;
	CurrentFloorNormal = FVector::UpVector;
	WallRideMinSpeed = 500.f;
	WallRideFriction = 0.2f;
	WallRideJumpForce = 800.f;
	WallRideJumpUpForce = 600.f;
	DashSpeed = 4000.f;
	DashDistance = 800.f;
	DashBoost = 300.f;
	DashVerticalInfluence = 0.4f;
	DashCooldown = 1.5f;
	LastDashTime = -9999.f;
	bHasDashedInAir = false;
	MaxWallJumps = 2;
	WallJumpCount = 0;
	MaxGrappleDistance = 5000.f;
	GrapplePullSpeed = 2000.f;
	GrappleCooldown = 2.f;
	LastGrappleTime = -9999.f;
	bHasGrappledInAir = false;
	GrappleEndBoost = 500.f;
}

void UAC_BaseMovement::BeginPlay()
{
	// Initialize references to the player's capsule component and visual mesh
	Super::BeginPlay();

	if (PawnOwner)
	{
		PlayerCapsule = Cast<UCapsuleComponent>(UpdatedComponent);
		PlayerVisualMesh = PawnOwner->FindComponentByClass<USkeletalMeshComponent>();
		GrappleCable = PawnOwner->FindComponentByClass<UCableComponent>();
	}
}

void UAC_BaseMovement::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//Check if valid pawn
	if (!PawnOwner || !UpdatedComponent || ShouldSkipUpdate(DeltaTime))
	{
		return;
	}

	//We put gravity on state that need it
	if (CurrentMovementState != EMovementState::WallRide && CurrentMovementState != EMovementState::Dashing && CurrentMovementState != EMovementState::Grapple)
		Velocity.Z += GetWorld()->GetGravityZ() * DeltaTime;

	// Get the current input vector and clamp it to max 1
	FVector InputVector = ConsumeInputVector().GetClampedToMaxSize(1.f);

	// Every Velocity is handled here
	UpdateVelocity(DeltaTime, InputVector);

	//	Every Rotation is handled here
	UpdateRotation(DeltaTime, InputVector);
	
	// Every Movement is handled here
	UpdateMovement(DeltaTime, InputVector);

	// Update the state machine
	UpdateState(DeltaTime);
}

void UAC_BaseMovement::ResizeCapsule(float NewHalfHeight)
{
	// Resize the capsule and adjust the visual mesh position accordingly
	if (PlayerCapsule && PlayerVisualMesh)
	{
		float CurrentHalfHeight = PlayerCapsule->GetUnscaledCapsuleHalfHeight();
		float HeightDifference = CurrentHalfHeight - NewHalfHeight;

		if (HeightDifference > 0.f)
		{
			PlayerCapsule->SetCapsuleSize(PlayerCapsule->GetUnscaledCapsuleRadius(), NewHalfHeight, true);
			PlayerCapsule->AddLocalOffset(FVector(0.f, 0.f, -HeightDifference));
		}
		else if (HeightDifference < 0.f)
		{
			PlayerCapsule->AddWorldOffset(FVector(0.f, 0.f, -HeightDifference));
			PlayerCapsule->SetCapsuleSize(PlayerCapsule->GetUnscaledCapsuleRadius(), NewHalfHeight, true);
		}

		if (PlayerVisualMesh)
		{
			PlayerVisualMesh->AddLocalOffset(FVector(0.f, 0.f, HeightDifference));
		}
	}
}

void UAC_BaseMovement::EnterState(EMovementState NewState)
{
	// Handle state transitions and perform any necessary actions when entering a new state
	EMovementState OldState = CurrentMovementState;
	CurrentMovementState = NewState;
	
	bool bWasCrouched = (OldState == EMovementState::Crouching || OldState == EMovementState::Sliding);
	bool bIsCrouched = (NewState == EMovementState::Crouching || NewState == EMovementState::Sliding);

	if (!bWasCrouched && bIsCrouched)
	{
		ResizeCapsule(CrouchHalfHeight);
	}
	else if (bWasCrouched && !bIsCrouched)
	{
		ResizeCapsule(StandHalfHeight);
	}

	if (OldState == EMovementState::Grapple)
	{
		OnGrappleStopped.Broadcast();
	}
}

void UAC_BaseMovement::Move(FVector2D MovementVector)
{
	// Convert the 2D movement vector into world space based on the controller's rotation
	if (PawnOwner != nullptr && PawnOwner->GetController() != nullptr)
	{
		const FRotator Rotation = PawnOwner->GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		PawnOwner->AddMovementInput(ForwardDirection, MovementVector.Y);
		PawnOwner->AddMovementInput(RightDirection, MovementVector.X);
	}
}

void UAC_BaseMovement::Look(FVector2D LookVector)
{
	// Apply the look input to the controller's rotation, scaled by camera sensitivity
	if (PawnOwner != nullptr && PawnOwner->GetController() != nullptr)
	{
		{
			PawnOwner->AddControllerYawInput(LookVector.X * CameraSensitivity);
			PawnOwner->AddControllerPitchInput(LookVector.Y * CameraSensitivity);
		}
	}
}

void UAC_BaseMovement::TriggerJump()
{
	// Handle jump input based on the current movement state
	if (CurrentMovementState == EMovementState::WallRide)
	{
		if (WallJumpCount >= MaxWallJumps)
			return;

		WallJumpCount++;
		Velocity += CurrentWallNormal * WallRideJumpForce;
		Velocity.Z = WallRideJumpUpForce;

		bIsGrounded = false;
		EnterState(EMovementState::InAir);
		return;
	}

	if (!CanStand()) return;

	if (CurrentMovementState == EMovementState::Idle || CurrentMovementState == EMovementState::Running || CurrentMovementState == EMovementState::Sliding || CurrentMovementState == EMovementState::Crouching)
	{
		if (Velocity.Z < 0.f) Velocity.Z = 0.f;
		Velocity.Z += JumpForce;
		bIsGrounded = false;
		EnterState(EMovementState::InAir);
	}
}

void UAC_BaseMovement::StartCrouchSlide()
{
	// Handle crouch/slide input based on the current movement state
	bWantsToCrouch = true;

	if (!bIsGrounded || CurrentMovementState == EMovementState::Sliding || CurrentMovementState == EMovementState::Crouching) return;

	if (Velocity.Size2D() > SlideThreshold && CurrentMovementState == EMovementState::Running)
	{
		Velocity.X *= SlideMultiplier;
		Velocity.Y *= SlideMultiplier;
		EnterState(EMovementState::Sliding);
	}
	else
		EnterState(EMovementState::Crouching);
}

void UAC_BaseMovement::StopCrouchSlide()
{
	// Handle stop crouch/slide input based on the current movement state
	bWantsToCrouch = false;

	if ((CurrentMovementState == EMovementState::Sliding || CurrentMovementState == EMovementState::Crouching) && CanStand())
	{
		if (Velocity.Size2D() > 0.f) 
			EnterState(EMovementState::Running);
		else 
			EnterState(EMovementState::Idle);
	}
}

bool UAC_BaseMovement::CanStand()
{
	// Check if the player can stand up from crouching by performing a capsule overlap test
	if (!PawnOwner || !PlayerCapsule) return true;

	FVector CurrentLocation = PlayerCapsule->GetComponentLocation();
	float HeightDifference = StandHalfHeight - CrouchHalfHeight;

	FVector TargetLocation = CurrentLocation + FVector(0.f, 0.f, HeightDifference + 2.f);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(PawnOwner);
	FCollisionShape Shape = FCollisionShape::MakeCapsule(PlayerCapsule->GetUnscaledCapsuleRadius() - 2.f, StandHalfHeight - 2.f);
	bool bHit = GetWorld()->OverlapAnyTestByChannel(TargetLocation, FQuat::Identity, ECC_Visibility, Shape, QueryParams);

	return !bHit;
}

void UAC_BaseMovement::TriggerDash(FVector2D DashInput)
{
	// Handle dash input based on the current movement state and cooldown
	if (!PawnOwner) return;
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastDashTime < DashCooldown) return;
	if (!bIsGrounded && bHasDashedInAir) return;
	DashDirection = FVector::ZeroVector;

	if (PawnOwner->GetController())
	{
		FRotator ControlRotation = PawnOwner->GetController()->GetControlRotation();
		FRotator YawRot(0.f, ControlRotation.Yaw, 0.f);
		FVector ForwardDirection = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
		FVector RightDirection = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
		
		if (!bIsGrounded)
		{
			if (DashInput.IsNearlyZero() || DashInput.Y > 0.5f)
			{
				FVector CameraForward = ControlRotation.Vector();
				DashDirection = FVector(CameraForward.X, CameraForward.Y, CameraForward.Z * DashVerticalInfluence);
				DashDirection.Normalize();
			}
			else
			{
				DashDirection = (ForwardDirection * DashInput.Y + RightDirection * DashInput.X).GetSafeNormal();
				DashDirection.Z = 0.f;
			}
		}
		else
		{
			if (DashInput.IsNearlyZero())
				DashDirection = PawnOwner->GetActorForwardVector();
			else
				DashDirection = (ForwardDirection * DashInput.Y + RightDirection * DashInput.X).GetSafeNormal();
			DashDirection.Z = 0.f;
		}
	}

	DashDirection.Normalize();
	PreDashVelocity = Velocity;
	DashDistanceRemaining = DashDistance;

	EnterState(EMovementState::Dashing);

	LastDashTime = CurrentTime;
	if (!bIsGrounded)
		bHasDashedInAir = true;
}

void UAC_BaseMovement::UpdateRotation(float DeltaTime, FVector InputVector)
{
	// This Function has as goal to check every movement state that has a need to change rotation and apply the correct rotation

	switch (CurrentMovementState)
	{
		case EMovementState::WallRide:
			WallRideRotation(DeltaTime, InputVector);
			break;

		case EMovementState::Dashing:
			DashingRotation(DeltaTime, InputVector);
			break;

		case EMovementState::Sliding:
			SlidingRotation(DeltaTime, InputVector);
			break;

		case EMovementState::Grapple:
			GrappleRotation(DeltaTime, InputVector);
			break;

		default:
			DefaultRotation(DeltaTime, InputVector);
	}
}

void UAC_BaseMovement::UpdateVelocity(float DeltaTime, FVector InputVector)
{
	// This Function has as goal to check every movement state that has a need to change velocity and apply the correct velocity
	switch (CurrentMovementState)
	{
		case EMovementState::WallRide:
			WallRideVelocity(DeltaTime, InputVector);
			break;
		
		case EMovementState::Sliding:
			SlidingVelocity(DeltaTime, InputVector);
			break;

		case EMovementState::Dashing:
			DashingVelocity(DeltaTime, InputVector);
			break;

		case EMovementState::Grapple:
			GrappleVelocity(DeltaTime, InputVector);
			break;

		case EMovementState::InAir:
			InAirVelocity(DeltaTime, InputVector);
			break;

		default:
			DefaultVelocity(DeltaTime, InputVector);
	}
}

void UAC_BaseMovement::UpdateMovement(float DeltaTime, FVector InputVector)
{
	// This function handles the actual movement of the character based on the current velocity and state

	// Calculate the desired movement for this frame
	FVector DesiredMovementThisFrame = Velocity * DeltaTime;

	// If the character is grounded and the floor normal is not too steep, project the movement onto the floor plane
	if (bIsGrounded && CurrentFloorNormal.Z > 0.7f && CurrentFloorNormal.Z < 0.99f)
		DesiredMovementThisFrame = FVector::VectorPlaneProject(DesiredMovementThisFrame, CurrentFloorNormal);

	// Move the character and handle collisions
	if (!DesiredMovementThisFrame.IsNearlyZero())
	{
		FHitResult Hit;
		SafeMoveUpdatedComponent(DesiredMovementThisFrame, UpdatedComponent->GetComponentQuat(), true, Hit);
		if (Hit.IsValidBlockingHit())
		{
			// If the character is wall riding and hits a wall that is too steep, transition to in-air state
			if (CurrentMovementState == EMovementState::WallRide)
			{
				if (FMath::Abs(Hit.Normal.Z) < 0.2f && FVector::DotProduct(DesiredMovementThisFrame.GetSafeNormal(), Hit.Normal) < -0.7f)
				{
					EnterState(EMovementState::InAir);
					Velocity = FVector::VectorPlaneProject(Velocity, Hit.Normal);
				}
			}

			SlideAlongSurface(DesiredMovementThisFrame, 1.f - Hit.Time, Hit.Normal, Hit);

			// If the character touch a roof
			if (Hit.Normal.Z < -0.7f)
				Velocity.Z = 0.f;
		}
	}
}

void UAC_BaseMovement::UpdateState(float DeltaTime)
{
	// This function checks the character's state and updates it based on the environment and input

	// Check if the character is grounded by performing a line trace downwards
	FVector Start = UpdatedComponent->GetComponentLocation();
	float TraceDistance = UpdatedComponent->Bounds.BoxExtent.Z + JumpCheckDistance;
	FVector End = Start - FVector(0.f, 0.f, TraceDistance);

	FHitResult FloorHit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(PawnOwner);
	bool bHit = GetWorld()->LineTraceSingleByChannel(FloorHit, Start, End, ECC_Visibility, QueryParams);

	// If character is ground and the floor normal is suitable, snap to the ground and reset relevant variables
	if (bHit && FloorHit.Normal.Z > 0.7f && Velocity.Z <= 0.f)
	{
		bIsGrounded = true;
		Velocity.Z = 0.f;
		CurrentFloorNormal = FloorHit.Normal;
		bHasDashedInAir = false;
		bHasGrappledInAir = false;
		WallJumpCount = 0;
		FVector SnapDelta(0.f, 0.f, -JumpCheckDistance);
		FHitResult SnapHit;
		SafeMoveUpdatedComponent(SnapDelta, UpdatedComponent->GetComponentQuat(), true, SnapHit);
	}

	else
	{
		bIsGrounded = false;
		CurrentFloorNormal = FVector::UpVector;
	}

	// Update the movement state based on whether the character is grounded or in the air
	if (bIsGrounded)
	{
		if (CurrentMovementState == EMovementState::Crouching && Velocity.Size2D() > (CrouchSpeed + 20.f))
		{
			EnterState(EMovementState::Sliding);
		}

		if (CurrentMovementState != EMovementState::Sliding && CurrentMovementState != EMovementState::Crouching && CurrentMovementState != EMovementState::Dashing && CurrentMovementState != EMovementState::Grapple)
		{
			if (Velocity.X == 0.f && Velocity.Y == 0.f && CurrentMovementState != EMovementState::Idle)
				EnterState(EMovementState::Idle);

			else if (CurrentMovementState != EMovementState::Running)
				EnterState(EMovementState::Running);
		}
		else
		{
			if (!bWantsToCrouch && CanStand() && CurrentMovementState != EMovementState::Dashing)
			{
				if (Velocity.Size2D() > 0.f)
					EnterState(EMovementState::Running);
				else
					EnterState(EMovementState::Idle);
			}
		}
	}

	// If the character is in the air, check for wall ride opportunities
	else
	{
		FVector WallStart = UpdatedComponent->GetComponentLocation();
		FVector Right = PawnOwner->GetActorRightVector();
		FHitResult LeftHit, RightHit, WallHit;
		FCollisionQueryParams WallParams; WallParams.AddIgnoredActor(PawnOwner);

		float WallTraceDistance = 60.f;

		bool bHitRight = GetWorld()->LineTraceSingleByChannel(RightHit, WallStart, WallStart + Right * WallTraceDistance, ECC_Visibility, WallParams);
		bool bHitLeft = GetWorld()->LineTraceSingleByChannel(LeftHit, WallStart, WallStart - Right * WallTraceDistance, ECC_Visibility, WallParams);
		bool bFoundWall = false;
		if (bHitRight)
		{
			WallHit = RightHit;
			bFoundWall = true;
		}
		else if (bHitLeft)
		{
			WallHit = LeftHit;
			bFoundWall = true;
		}

		if (bFoundWall && FMath::Abs(WallHit.Normal.Z) < 0.2f)
		{
			float ApproachDot = FVector::DotProduct(Velocity.GetSafeNormal2D(), -WallHit.Normal);
			if (ApproachDot > -0.15f && ApproachDot < 0.5f && Velocity.Size2D() >= WallRideMinSpeed && CurrentMovementState != EMovementState::Dashing && WallJumpCount < MaxWallJumps)
			{
				CurrentWallNormal = WallHit.Normal;

				if (CurrentMovementState != EMovementState::WallRide)
				{
					EnterState(EMovementState::WallRide);
					Velocity.Z = 0.f;
					bHasDashedInAir = false;
				}
			}
			else if (CurrentMovementState == EMovementState::WallRide)
			{
				EnterState(EMovementState::InAir);
			}
		}
		else
		{
			if (CurrentMovementState != EMovementState::InAir && CurrentMovementState != EMovementState::Dashing && CurrentMovementState != EMovementState::Grapple)
				EnterState(EMovementState::InAir);
		}
	}
}

void UAC_BaseMovement::WallRideRotation(float DeltaTime, FVector InputVector)
{
	// Handle WallRide Rotation : Set the rotation along the wall normal

	// Calculate the target rotation based on the wall normal and current velocity
	FRotator TargetRotation = Velocity.Rotation();
	TargetRotation.Pitch = 0.f;
	FRotator SmoothRotation = FMath::RInterpTo(PawnOwner->GetActorRotation(), TargetRotation, DeltaTime, 12.f);
	PawnOwner->SetActorRotation(SmoothRotation);
}

void UAC_BaseMovement::SlidingRotation(float DeltaTime, FVector InputVector)
{
	// Handle Sliding Rotation : Set the rotation in the direction of the slide along the floor normal
	FRotator TargetRotation = FRotationMatrix::MakeFromZX(CurrentFloorNormal, Velocity).Rotator();
	FRotator SmoothRotation = FMath::RInterpTo(PawnOwner->GetActorRotation(), TargetRotation, DeltaTime, 10.f);
	PawnOwner->SetActorRotation(SmoothRotation);
}

void UAC_BaseMovement::DashingRotation(float DeltaTime, FVector InputVector)
{
	// Handle Dashing Rotation : Set the rotation in the direction of the dash
	FRotator TargetRotation = Velocity.Rotation();
	TargetRotation.Pitch = 0.f;
	FRotator SmoothRotation = FMath::RInterpTo(PawnOwner->GetActorRotation(), TargetRotation, DeltaTime, 20.f);
	PawnOwner->SetActorRotation(SmoothRotation);
}

void UAC_BaseMovement::GrappleRotation(float DeltaTime, FVector InputVector)
{
	// Handle Grapple Rotation : Set the rotation in the direction of the grapple point
	FRotator TargetRotation = Velocity.Rotation();
	TargetRotation.Pitch = 0.f;
	FRotator SmoothRotation = FMath::RInterpTo(PawnOwner->GetActorRotation(), TargetRotation, DeltaTime, 20.f);
	PawnOwner->SetActorRotation(SmoothRotation);
}

void UAC_BaseMovement::DefaultRotation(float DeltaTime, FVector InputVector)
{
	// Handle Default Rotation : Set the rotation based on input vector and camera direction

	// Get the current rotation of the pawn and zero out the pitch and roll
	FRotator TargetRotation = PawnOwner->GetActorRotation();
	TargetRotation.Pitch = 0.f;
	TargetRotation.Roll = 0.f;

	// If there is input, set the target rotation to face the direction of movement
	if (!InputVector.IsNearlyZero())
	{
		FRotator ControlRotation = PawnOwner->GetController()->GetControlRotation();
		TargetRotation.Yaw = ControlRotation.Yaw;
	}

	// Smoothly interpolate the rotation towards the target rotation
	FRotator SmoothRotation = FMath::RInterpTo(PawnOwner->GetActorRotation(), TargetRotation, DeltaTime, 12.f);
	PawnOwner->SetActorRotation(SmoothRotation);
}

void UAC_BaseMovement::WallRideVelocity(float DeltaTime, FVector InputVector)
{
	// Handle WallRide Velocity : Project the velocity along the wall normal and apply friction, then snap to the wall

	// Perform a line trace to find the wall normal and update the current wall normal
	FVector Start = UpdatedComponent->GetComponentLocation();
	FVector End = Start - (CurrentWallNormal * 50.f);
	FHitResult WallHit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(PawnOwner);

	// Interpolate the current wall normal towards the hit normal for smoother movement
	if (GetWorld()->LineTraceSingleByChannel(WallHit, Start, End, ECC_Visibility, QueryParams))
		CurrentWallNormal = FMath::VInterpNormalRotationTo(CurrentWallNormal, WallHit.Normal, DeltaTime, 15.f);

	// Project the velocity onto the plane defined by the wall normal and apply friction
	Velocity = FVector::VectorPlaneProject(Velocity, CurrentWallNormal);
	Velocity.Z = 0.f;
	float CurrentSpeed = Velocity.Size();
	CurrentSpeed -= CurrentSpeed * WallRideFriction * DeltaTime;
	Velocity = Velocity.GetSafeNormal() * CurrentSpeed;

	// Snap the character to the wall to prevent drifting away
	FVector SnapToWall = -CurrentWallNormal * 10.f;
	FHitResult SnapHit;
	SafeMoveUpdatedComponent(SnapToWall, UpdatedComponent->GetComponentQuat(), true, SnapHit);
}

void UAC_BaseMovement::SlidingVelocity(float DeltaTime, FVector InputVector)
{
	// Handle Sliding Velocity : Apply slope push and friction, then check if the slide should stop
	FVector HorizontalVelocity = FVector(Velocity.X, Velocity.Y, 0.f);
	FVector SlopePush = FVector(CurrentFloorNormal.X, CurrentFloorNormal.Y, 0.f) * SlideGravityForce * DeltaTime;
	HorizontalVelocity += SlopePush;
	HorizontalVelocity -= HorizontalVelocity * SlideFriction * DeltaTime;

	Velocity.X = HorizontalVelocity.X;
	Velocity.Y = HorizontalVelocity.Y;

	// If velocity is below the slide stop speed, transition to crouching state
	if (HorizontalVelocity.Size() < SlideStopSpeed)
	{
		EnterState(EMovementState::Crouching);
	}
}

void UAC_BaseMovement::DashingVelocity(float DeltaTime, FVector InputVector)
{
	// Handle Dashing Velocity : Move the character in the dash direction and check if the dash distance has been covered
	float MoveDistance = DashSpeed * DeltaTime;

	if (DashDistanceRemaining <= MoveDistance)
	{
		Velocity = DashDirection * (PreDashVelocity.Size() + DashBoost);
		EnterState(EMovementState::InAir);
	}
	else
	{
		Velocity = DashDirection * DashSpeed;
		DashDistanceRemaining -= MoveDistance;
	}
}

void UAC_BaseMovement::GrappleVelocity(float DeltaTime, FVector InputVector)
{
	// Handle Grapple Velocity : Move the character towards the grapple point and check if the grapple should stop
	FVector StartLoc = UpdatedComponent->GetComponentLocation();
	FVector ToGrapple = GrapplePoint - StartLoc;
	float Distance = ToGrapple.Size();

	if (Distance < 150.f)
	{
		StopGrapple();
	}
	else
	{
		Velocity = ToGrapple.GetSafeNormal() * GrapplePullSpeed;
	}
}

void UAC_BaseMovement::InAirVelocity(float DeltaTime, FVector InputVector)
{
	// Handle In Air Velocity : Apply air steering and braking based on input vector
	if (!InputVector.IsNearlyZero())
	{
		FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.f);
		float CurrentSpeed = HorizontalVelocity.Size();

		// If the current speed is below a threshold, apply full input to accelerate
		if (CurrentSpeed < 10.f)
		{
			Velocity.X += InputVector.X * RunSpeed * DeltaTime;
			Velocity.Y += InputVector.Y * RunSpeed * DeltaTime;
		}
		else
		{
			FVector VelocityDir = HorizontalVelocity.GetSafeNormal();
			FVector InputDir = InputVector.GetSafeNormal();
			float Dot = FVector::DotProduct(VelocityDir, InputDir);

			// If the input is opposite to the current velocity, apply air braking
			if (Dot <= -0.966f)
			{
				CurrentSpeed += (CurrentSpeed * Dot * AirBrakeFriction * DeltaTime);
				CurrentSpeed = FMath::Max(0.f, CurrentSpeed);
				Velocity.X = VelocityDir.X * CurrentSpeed;
				Velocity.Y = VelocityDir.Y * CurrentSpeed;
			}

			// If the input is not opposite, interpolate the velocity direction towards the input direction
			else
			{
				FVector NewDir = FMath::VInterpNormalRotationTo(VelocityDir, InputDir, DeltaTime, AirSteerSpeed);

				if (Dot < 0.f)
				{
					CurrentSpeed += (CurrentSpeed * Dot * AirBrakeFriction * DeltaTime);
					CurrentSpeed = FMath::Max(0.f, CurrentSpeed);
				}

				Velocity.X = NewDir.X * CurrentSpeed;
				Velocity.Y = NewDir.Y * CurrentSpeed;
			}
		}
	}
}

void UAC_BaseMovement::DefaultVelocity(float DeltaTime, FVector InputVector)
{
	// Handle Default Velocity : Apply acceleration and deceleration based on input vector and current movement state
	float CurrentMaxSpeed = (CurrentMovementState == EMovementState::Crouching) ? CrouchSpeed : RunSpeed;
	FVector TargetVelocity = InputVector * CurrentMaxSpeed;
	FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.f);

	if (InputVector.IsNearlyZero())
	{
		// If there is no input, decelerate the character to a stop
		FVector NewVelocity = FMath::VInterpTo(HorizontalVelocity, FVector::ZeroVector, DeltaTime, GroundDeceleration * 1.5f);

		// If the new velocity is below a threshold, set it to zero to prevent sliding
		if (NewVelocity.Size2D() < 15.f)
		{
			Velocity.X = 0.f;
			Velocity.Y = 0.f;
		}
		else
		{
			Velocity.X = NewVelocity.X;
			Velocity.Y = NewVelocity.Y;
		}
	}

	// If the current speed is above the max speed, decelerate towards the target velocity
	else if (HorizontalVelocity.Size2D() > CurrentMaxSpeed)
	{
		FVector NewVelocity = FMath::VInterpTo(HorizontalVelocity, TargetVelocity, DeltaTime, GroundDeceleration);
		Velocity.X = NewVelocity.X;
		Velocity.Y = NewVelocity.Y;
	}

	else
	{
		Velocity.X = TargetVelocity.X;
		Velocity.Y = TargetVelocity.Y;
	}
}


void UAC_BaseMovement::StartGrapple()
{
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastGrappleTime < GrappleCooldown) return;
	if (!bIsGrounded && bHasGrappledInAir) return;
	FVector Start;
	FRotator CamRotation;
	PawnOwner->GetController()->GetPlayerViewPoint(Start, CamRotation);
	FVector Forward = CamRotation.Vector();
	FVector End = Start + Forward * MaxGrappleDistance;

	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(PawnOwner);

	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams))
	{
		LastGrappleTime = CurrentTime;
		if (!bIsGrounded)
			bHasGrappledInAir = true;

		GrapplePoint = Hit.ImpactPoint;

		if (GrappleAnchorClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = PawnOwner;
			if (GrappleAnchorActor)
			{
				GrappleAnchorActor->Destroy();
				GrappleAnchorActor = nullptr;
			}
			GrappleAnchorActor = GetWorld()->SpawnActor<AActor>(GrappleAnchorClass, GrapplePoint, FRotator::ZeroRotator, SpawnParams);
			GrappleCable->SetAttachEndTo(GrappleAnchorActor, NAME_None, NAME_None);
		}

		EnterState(EMovementState::Grapple);
		OnGrappleStarted.Broadcast();
	}
}

void UAC_BaseMovement::StopGrapple()
{
	if (CurrentMovementState == EMovementState::Grapple)
	{
		if (!bIsGrounded)
		{
			Velocity.Z = GrappleEndBoost;
			EnterState(EMovementState::InAir);
			OnGrappleStopped.Broadcast();
		}
		else
			EnterState(EMovementState::Running);

		if (GrappleAnchorActor)
		{
			GrappleAnchorActor->Destroy();
			GrappleAnchorActor = nullptr;
		}
	}
}