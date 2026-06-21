// Fill out your copyright notice in the Description page of Project Settings.


#include "UAC_BaseMovement.h"

UAC_BaseMovement::UAC_BaseMovement()
{
	PrimaryComponentTick.bCanEverTick = true;
	CurrentMovementState = EMovementState::Idle;
	CameraSensitivity = 1.f;
	WalkSpeed = 600.f;
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
	DashForce = 2500.f;
	DashCooldown = 1.5f;
	DashVerticalInfluence = 0.4f;
	GroundDashMultiplier = 2.f;
	LastDashTime = -9999.f;
	bHasDashedInAir = false;
	WallRideMinSpeed = 500.f;
	WallRideFriction = 0.2f;
	WallRideJumpForce = 800.f;
	WallRideJumpUpForce = 600.f;

}

void UAC_BaseMovement::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!PawnOwner || !UpdatedComponent || ShouldSkipUpdate(DeltaTime))
	{
		return;
	}

	UpdateState(DeltaTime);

	if (CurrentMovementState != EMovementState::WallRide)
		Velocity.Z += GetWorld()->GetGravityZ() * DeltaTime;

	FVector InputVector = ConsumeInputVector().GetClampedToMaxSize(1.f);

	if (CurrentMovementState == EMovementState::WallRide)
	{
		FRotator TargetRotation = Velocity.Rotation();
		TargetRotation.Pitch = 0.f;
		FRotator SmoothRotation = FMath::RInterpTo(PawnOwner->GetActorRotation(), TargetRotation, DeltaTime, 12.f);
		PawnOwner->SetActorRotation(SmoothRotation);
	}
	else if (!InputVector.IsNearlyZero() && CurrentMovementState != EMovementState::Sliding)
	{
		if (PawnOwner->GetController())
		{
			FRotator ControlRotation = PawnOwner->GetController()->GetControlRotation();
			FRotator TargetRotation = FRotator(0.f, ControlRotation.Yaw, 0.f);
			FRotator SmoothRotation = FMath::RInterpTo(PawnOwner->GetActorRotation(), TargetRotation, DeltaTime, 12.f);
			PawnOwner->SetActorRotation(SmoothRotation);
		}
	}

	if (CurrentMovementState == EMovementState::Sliding)
	{
		FVector HorizontalVelocity = FVector(Velocity.X, Velocity.Y, 0.f);
		FVector SlopePush = FVector(CurrentFloorNormal.X, CurrentFloorNormal.Y, 0.f) * SlideGravityForce * DeltaTime;
		HorizontalVelocity += SlopePush;
		HorizontalVelocity -= HorizontalVelocity * SlideFriction * DeltaTime;

		Velocity.X = HorizontalVelocity.X;
		Velocity.Y = HorizontalVelocity.Y;

		if (HorizontalVelocity.Size() < SlideStopSpeed)
		{
			EnterState(EMovementState::Crouching);
		}
	}

	else if (CurrentMovementState == EMovementState::WallRide)
	{
		FVector Start = UpdatedComponent->GetComponentLocation();
		FHitResult WallHit;
		FCollisionQueryParams QueryParams; 
		QueryParams.AddIgnoredActor(PawnOwner);
		if (GetWorld()->LineTraceSingleByChannel(WallHit, Start, Start - CurrentWallNormal * 100.f, ECC_Visibility, QueryParams))
			CurrentWallNormal = WallHit.Normal;
		Velocity = FVector::VectorPlaneProject(Velocity, CurrentWallNormal);
		Velocity.Z = 0.f;
		float CurrentSpeed = Velocity.Size2D();
		CurrentSpeed -= CurrentSpeed * WallRideFriction * DeltaTime;
		Velocity = Velocity.GetSafeNormal() * CurrentSpeed;
		FVector SnapToWall = -CurrentWallNormal * 10.f;
		FHitResult SnapHit;
		SafeMoveUpdatedComponent(SnapToWall, UpdatedComponent->GetComponentQuat(), true, SnapHit);
	}

	else if (CurrentMovementState == EMovementState::InAir)
	{
		if (!InputVector.IsNearlyZero())
		{
			FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.f);
			float CurrentSpeed = HorizontalVelocity.Size();
			if (CurrentSpeed < 10.f)	
			{
				Velocity.X += InputVector.X * WalkSpeed * DeltaTime;
				Velocity.Y += InputVector.Y * WalkSpeed * DeltaTime;
			}
			else
			{
				FVector VelocityDir = HorizontalVelocity.GetSafeNormal();
				FVector InputDir = InputVector.GetSafeNormal();
				float Dot = FVector::DotProduct(VelocityDir, InputDir);
				
				if (Dot <= -0.966f)
				{
					CurrentSpeed += (CurrentSpeed * Dot * AirBrakeFriction * DeltaTime);
					CurrentSpeed = FMath::Max(0.f, CurrentSpeed);
					Velocity.X = VelocityDir.X * CurrentSpeed;
					Velocity.Y = VelocityDir.Y * CurrentSpeed;
				}

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

	else
	{
		float CurrentMaxSpeed = (CurrentMovementState == EMovementState::Crouching) ? CrouchSpeed : WalkSpeed;
		FVector TargetVelocity = InputVector * CurrentMaxSpeed;
		FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.f);

		if (InputVector.IsNearlyZero())
		{
			Velocity.X = 0.f;
			Velocity.Y = 0.f;
		}
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
	

	// We do the deplacement
	FVector DesiredMovementThisFrame = Velocity * DeltaTime;
	if (bIsGrounded && CurrentFloorNormal.Z > 0.7f && CurrentFloorNormal.Z < 0.99f)
	{
		DesiredMovementThisFrame = FVector::VectorPlaneProject(DesiredMovementThisFrame, CurrentFloorNormal);
	}
	if (!DesiredMovementThisFrame.IsNearlyZero())
	{
		FHitResult Hit;
		SafeMoveUpdatedComponent(DesiredMovementThisFrame, UpdatedComponent->GetComponentQuat(), true, Hit);
		// We get whatever hit our pawn then check if a ceiling is hitted to adjust the veloctiy when jumping
		if (Hit.IsValidBlockingHit())
		{
			SlideAlongSurface(DesiredMovementThisFrame, 1.f - Hit.Time, Hit.Normal, Hit);

			if (Hit.Normal.Z < -0.7f) Velocity.Z = 0.f;
		}
	}

	FVector Start = UpdatedComponent->GetComponentLocation();
	float TraceDistance = UpdatedComponent->Bounds.BoxExtent.Z + JumpCheckDistance;
	FVector End = Start - FVector(0.f, 0.f, TraceDistance);

	FHitResult FloorHit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(PawnOwner);
	bool bHit = GetWorld()->LineTraceSingleByChannel(FloorHit, Start, End, ECC_Visibility, QueryParams);

	if (bHit && FloorHit.Normal.Z > 0.7f && Velocity.Z <= 0.f) 
	{
		bIsGrounded = true;
		Velocity.Z = 0.f;
		CurrentFloorNormal = FloorHit.Normal;
		bHasDashedInAir = false;
		FVector SnapDelta(0.f, 0.f, -JumpCheckDistance);
		FHitResult SnapHit;
		SafeMoveUpdatedComponent(SnapDelta, UpdatedComponent->GetComponentQuat(), true, SnapHit);
	}
	else
	{
		bIsGrounded = false;
		CurrentFloorNormal = FVector::UpVector;
	}

	if (bIsGrounded)
	{
		if (CurrentMovementState != EMovementState::Sliding && CurrentMovementState != EMovementState::Crouching)
		{
			if (Velocity.X == 0.f && Velocity.Y == 0.f && CurrentMovementState != EMovementState::Idle) 
				EnterState(EMovementState::Idle);

			else if (CurrentMovementState != EMovementState::Walking) 
				EnterState(EMovementState::Walking);
		}
		else
		{
			if (!bWantsToCrouch && CanStand())
			{
				if (Velocity.Size2D() > 0.f)
					EnterState(EMovementState::Walking);
				else
					EnterState(EMovementState::Idle);
			}
		}
	}
	
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
				if (ApproachDot < 0.5f && Velocity.Size2D() >= WallRideMinSpeed)
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
				if (CurrentMovementState != EMovementState::InAir)
					EnterState(EMovementState::InAir);
			}
	}
}

void UAC_BaseMovement::ResizeCapsule(float NewHalfHeight)
{
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

		float ScaleRatio = NewHalfHeight / CurrentHalfHeight;

		FVector CurrentScale = PlayerVisualMesh->GetRelativeScale3D();
		PlayerVisualMesh->SetRelativeScale3D(FVector(CurrentScale.X, CurrentScale.Y, CurrentScale.Z * ScaleRatio));
	}
}

void UAC_BaseMovement::EnterState(EMovementState NewState)
{
	EMovementState OldState = CurrentMovementState;
	ExitState(OldState);
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
}

void UAC_BaseMovement::UpdateState(float DeltaTime)
{
	// Implement state-specific logic here if needed
}

void UAC_BaseMovement::ExitState(EMovementState OldState)
{
	// Implement state exit logic here if needed
}

void UAC_BaseMovement::Move(FVector2D MovementVector)
{
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
	if (!CanStand()) return;
	if (CurrentMovementState == EMovementState::WallRide)
	{
		Velocity = CurrentWallNormal * WallRideJumpForce;
		Velocity.Z = WallRideJumpUpForce;

		bIsGrounded = false;
		EnterState(EMovementState::InAir);
		return;
	}
	if (CurrentMovementState == EMovementState::Idle || CurrentMovementState == EMovementState::Walking || CurrentMovementState == EMovementState::Sliding || CurrentMovementState == EMovementState::Crouching)
	{
		if (Velocity.Z < 0.f) Velocity.Z = 0.f;
		Velocity.Z += JumpForce;
		bIsGrounded = false;
		EnterState(EMovementState::InAir);
	}
}

void UAC_BaseMovement::StartCrouchSlide()
{
	bWantsToCrouch = true;

	if (!bIsGrounded || CurrentMovementState == EMovementState::Sliding || CurrentMovementState == EMovementState::Crouching) return;

	if (Velocity.Size2D() > SlideThreshold && CurrentMovementState == EMovementState::Walking)
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
	bWantsToCrouch = false;

	if ((CurrentMovementState == EMovementState::Sliding || CurrentMovementState == EMovementState::Crouching) && CanStand())
	{
		if (Velocity.Size2D() > 0.f) 
			EnterState(EMovementState::Walking);
		else 
			EnterState(EMovementState::Idle);
	}
}

bool UAC_BaseMovement::CanStand()
{
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

void UAC_BaseMovement::BeginPlay()
{
	Super::BeginPlay();

	if (PawnOwner)
	{
		PlayerCapsule = Cast<UCapsuleComponent>(UpdatedComponent);
		PlayerVisualMesh = PawnOwner->FindComponentByClass<UStaticMeshComponent>();
	}
}

void UAC_BaseMovement::TriggerDash(FVector2D DashInput)
{
	if (!PawnOwner) return;
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastDashTime < DashCooldown) return;
	if (!bIsGrounded && bHasDashedInAir) return;
	FVector DashDirection = FVector::ZeroVector;

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

	float FinalDashForce = bIsGrounded ? (DashForce * GroundDashMultiplier) : DashForce;
	Velocity = DashDirection * FinalDashForce;
	LastDashTime = CurrentTime;
	if (!bIsGrounded) bHasDashedInAir = true;
}