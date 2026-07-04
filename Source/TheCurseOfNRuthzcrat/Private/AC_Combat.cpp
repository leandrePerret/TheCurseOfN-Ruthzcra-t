#include "AC_Combat.h"

UAC_Combat::UAC_Combat()
{
	PrimaryComponentTick.bCanEverTick = true;
	BaseDamage = 20.f;
	FireRange = 10000.f;
	FireRate = 0.1f;
}
void UAC_Combat::BeginPlay()
{
	Super::BeginPlay();	
}

void UAC_Combat::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UAC_Combat::StartFire()
{
	PerformHit();

	if (FireRate > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(FireTimerHandle, this, &UAC_Combat::PerformHit, FireRate, true);
	}
}

void UAC_Combat::StopFire()
{
	GetWorld()->GetTimerManager().ClearTimer(FireTimerHandle);
}


void UAC_Combat::PerformHit()
{
	APawn* PawnOwner = Cast<APawn>(GetOwner());
	if (!PawnOwner || !PawnOwner->GetController())
		return;
	
	FVector StartLocation;
	FRotator CamRotation;
	PawnOwner->GetController()->GetPlayerViewPoint(StartLocation, CamRotation);
	FVector EndLocation = StartLocation + (CamRotation.Vector() * FireRange);

	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(PawnOwner);

	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, StartLocation, EndLocation, ECC_Visibility, QueryParams);

	FColor LineColor = bHit ? FColor::Red : FColor::Green;
	DrawDebugLine(GetWorld(), StartLocation, EndLocation, LineColor, false, 1.f, 0, 1.f);

	if (bHit)
	{
		DrawDebugBox(GetWorld(), Hit.ImpactPoint, FVector(5.f), FColor::Blue, false, 1.f, 0, 1.f);
	}
}