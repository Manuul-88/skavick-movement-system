// GeckoMovementComponent.cpp
#include "GeckoMovementComponent.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "DrawDebugHelpers.h"

UGeckoMovementComponent::UGeckoMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	MaxWalkSpeed = WalkSpeed;
}
 
bool UGeckoMovementComponent::IsClimbing() const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == CMOVE_Climbing;
}

bool UGeckoMovementComponent::TraceWall(FHitResult& OutHit) const
{
	if (!UpdatedComponent || !GetWorld())
	{
		return false;
	}

	FVector Start = UpdatedComponent->GetComponentLocation();
	FVector Dir = UpdatedComponent->GetForwardVector();

	const bool bInClimbMode = (MovementMode == MOVE_Custom && CustomMovementMode == CMOVE_Climbing);
	if (bInClimbMode && !CurrentWallNormal.IsNearlyZero())
	{
		const FVector N = CurrentWallNormal.GetSafeNormal();
		Dir = -N;
		Start += N * 5.f;
	}

	const FVector End = Start + (Dir.GetSafeNormal() * TraceDistance);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(GeckoClimbTrace), false);
	Params.AddIgnoredActor(GetOwner());

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		OutHit, Start, End, ECC_Visibility, Params);

	//DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Green : FColor::Red, false, 0.05f, 0, 1.5f);
	return bHit;
}

bool UGeckoMovementComponent::TraceClimbCandidate(FHitResult& OutHit) const
{
	if (!UpdatedComponent || !GetWorld())
	{
		return false;
	}

	const FVector Start = UpdatedComponent->GetComponentLocation();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(GeckoClimbCandidateTrace), false);
	Params.AddIgnoredActor(GetOwner());

	auto DoTrace = [&](const FVector& Dir) -> bool
		{
			const FVector End = Start + (Dir.GetSafeNormal() * TraceDistance);
			const bool bHit = GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, Params);
			DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Cyan : FColor::Red, false, 0.08f, 0, 1.2f);
			return bHit;
		};

	if (DoTrace(UpdatedComponent->GetForwardVector()))
	{
		return true;
	}

	if (bAllowCeilingClimb)
	{
		if (DoTrace(FVector::UpVector))
		{
			return true;
		}
	}

	return false;
}

void UGeckoMovementComponent::TryStartClimb()
{
	if (!bClimbEnabled)
	{
		return;
	}

	const bool bAuth = (PawnOwner && PawnOwner->HasAuthority());
	if (!bAuth)
	{
		return;
	}

	if (MovementMode == MOVE_Custom && CustomMovementMode == CMOVE_Climbing)
	{
		return;
	}

	if (!PawnOwner || !UpdatedComponent || !GetWorld())
	{
		return;
	}

	FHitResult Hit;
	if (!TraceClimbCandidate(Hit))
	{
		return;
	}

	const FVector N = Hit.ImpactNormal.GetSafeNormal();
	if (N.IsNearlyZero())
	{
		return;
	}

	const float DotUp = FVector::DotProduct(N, FVector::UpVector);
	const bool bIsWall = FMath::Abs(DotUp) <= WallDotAbsMax;
	const bool bIsCeiling = bAllowCeilingClimb && (DotUp <= CeilingDotThreshold);

	if (!(bIsWall || bIsCeiling))
	{
		return;
	}

	bIsClimbing = true;
	CurrentWallNormal = N;
	SmoothedWallNormal = N;
	LastWallDistance = Hit.Distance;

	StopMovementImmediately();
	Velocity = FVector::ZeroVector;

	SetMovementMode(MOVE_Custom, CMOVE_Climbing);
}

void UGeckoMovementComponent::StopClimb()
{
	const bool bAuth = (PawnOwner && PawnOwner->HasAuthority());
	const bool bOwnerClient = (PawnOwner && PawnOwner->IsLocallyControlled());

	bIsClimbing = false;

	StopMovementImmediately();
	Velocity = FVector::ZeroVector;
	CurrentWallNormal = FVector::ZeroVector;
	SmoothedWallNormal = FVector::ZeroVector;
	LastWallDistance = 0.f;

	if (bAuth || bOwnerClient)
	{
		SetMovementMode(MOVE_Walking);
	}
}

void UGeckoMovementComponent::UpdateWallNormalOrStop()
{
	const bool bAuth = (PawnOwner && PawnOwner->HasAuthority());

	FHitResult Hit;
	const bool bHit = TraceWall(Hit);

	if (!bHit)
	{
		if (bAuth)
		{
			StopClimb();
		}
		return;
	}

	CurrentWallNormal = Hit.ImpactNormal;
	LastWallDistance = Hit.Distance;

	if (CurrentWallNormal.GetSafeNormal().IsNearlyZero())
	{
		if (bAuth)
		{
			StopClimb();
		}
	}
}

void UGeckoMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UGeckoMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
	if (CustomMovementMode == CMOVE_Climbing)
	{
		PhysClimbing(DeltaTime, Iterations);
		return;
	}

	Super::PhysCustom(DeltaTime, Iterations);
}

float UGeckoMovementComponent::GetMaxSpeed() const
{
	float Result = WalkSpeed;

	if (MovementMode == MOVE_Custom && CustomMovementMode == CMOVE_Climbing)
	{
		Result = bSprintRequested ? (ClimbSpeed * ClimbSprintMultiplier) : ClimbSpeed;
	}
	else
	{
		Result = bSprintRequested ? SprintSpeed : WalkSpeed;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			2,
			0.f,
			FColor::Green,
			FString::Printf(
				TEXT("GetMaxSpeed | Sprint=%s | Speed=%.1f"),
				bSprintRequested ? TEXT("TRUE") : TEXT("FALSE"),
				Result
			)
		);
	}

	return Result;
}

void UGeckoMovementComponent::SetSprintRequested(bool bNewSprint)
{
	bSprintRequested = bNewSprint;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.f,
			FColor::Yellow,
			FString::Printf(
				TEXT("SetSprintRequested = %s"),
				bSprintRequested ? TEXT("TRUE") : TEXT("FALSE")
			)
		);
	}
}

void UGeckoMovementComponent::PhysClimbing(float DeltaTime, int32 Iterations)
{
	const bool bAuth = (PawnOwner && PawnOwner->HasAuthority());

	if (!PawnOwner || !UpdatedComponent || !GetWorld())
	{
		if (bAuth)
		{
			StopClimb();
		}
		return;
	}

	if (!(MovementMode == MOVE_Custom && CustomMovementMode == CMOVE_Climbing))
	{
		return;
	}

	UpdateWallNormalOrStop();
	if (!(MovementMode == MOVE_Custom && CustomMovementMode == CMOVE_Climbing))
	{
		return;
	}

	FVector WallN = CurrentWallNormal.GetSafeNormal();
	if (WallN.IsNearlyZero())
	{
		if (bAuth)
		{
			StopClimb();
		}
		return;
	}

	if (!SmoothedWallNormal.IsNearlyZero() &&
		FVector::DotProduct(SmoothedWallNormal, WallN) < 0.f)
	{
		WallN *= -1.f;
	}

	const float Alpha = 1.f - FMath::Exp(-NormalInterpSpeed * DeltaTime);
	if (SmoothedWallNormal.IsNearlyZero())
	{
		SmoothedWallNormal = WallN;
	}
	else
	{
		SmoothedWallNormal = FMath::Lerp(SmoothedWallNormal, WallN, Alpha).GetSafeNormal();
	}
	WallN = SmoothedWallNormal;

	if (WallN.IsNearlyZero())
	{
		if (bAuth)
		{
			StopClimb();
		}
		return;
	}

	const float DotUp = FVector::DotProduct(WallN, FVector::UpVector);
	const bool bIsCeiling = bAllowCeilingClimb && (DotUp <= CeilingDotThreshold);

	FRotator InputRot = FRotator::ZeroRotator;
	if (PawnOwner->Controller)
	{
		InputRot = PawnOwner->Controller->GetControlRotation();
	}
	else
	{
		InputRot = PawnOwner->GetActorRotation();
	}

	const FRotator YawOnly(0.f, InputRot.Yaw, 0.f);

	const FVector ControlFwd = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::X);
	const FVector ControlRight = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y);

	FVector ClimbUp = FVector::ZeroVector;
	FVector ClimbRight = FVector::ZeroVector;

	if (!bIsCeiling)
	{
		ClimbUp = FVector::VectorPlaneProject(FVector::UpVector, WallN).GetSafeNormal();
		if (ClimbUp.IsNearlyZero())
		{
			if (bAuth)
			{
				StopClimb();
			}
			return;
		}

		ClimbRight = FVector::CrossProduct(WallN, ClimbUp).GetSafeNormal();
		if (ClimbRight.IsNearlyZero())
		{
			if (bAuth)
			{
				StopClimb();
			}
			return;
		}
	}
	else
	{
		ClimbRight = FVector::VectorPlaneProject(ControlRight, WallN).GetSafeNormal();
		if (ClimbRight.IsNearlyZero())
		{
			ClimbRight = FVector::CrossProduct(WallN, FVector::UpVector).GetSafeNormal();
		}

		ClimbUp = FVector::CrossProduct(ClimbRight, WallN).GetSafeNormal();
		if (ClimbUp.IsNearlyZero())
		{
			if (bAuth)
			{
				StopClimb();
			}
			return;
		}

		ClimbUp *= -1.f;
	}

	const FVector Accel = GetCurrentAcceleration();
	FVector Accel2D(Accel.X, Accel.Y, 0.f);

	if (Accel2D.IsNearlyZero())
	{
		Velocity = FVector::ZeroVector;
	}
	else
	{
		Accel2D = Accel2D.GetSafeNormal();

		float InputForward = FVector::DotProduct(Accel2D, ControlFwd);
		float InputRight = FVector::DotProduct(Accel2D, ControlRight);

		if (FMath::Abs(InputForward) < ClimbInputDeadzone)
		{
			InputForward = 0.f;
		}

		if (FMath::Abs(InputRight) < ClimbInputDeadzone)
		{
			InputRight = 0.f;
		}

		FVector MoveDir = (ClimbUp * InputForward) + (ClimbRight * InputRight);
		MoveDir = MoveDir.GetClampedToMaxSize(1.f);

		Velocity = MoveDir * GetMaxSpeed();
	}

	FHitResult MoveHit;
	SafeMoveUpdatedComponent(Velocity * DeltaTime, UpdatedComponent->GetComponentQuat(), true, MoveHit);

	const float TooFar = DesiredWallDistance + WallDistanceTolerance;
	if (LastWallDistance > TooFar)
	{
		const float DeltaDist = LastWallDistance - DesiredWallDistance;
		const float Push = FMath::Min(SnapStrength, DeltaDist);

		FHitResult SnapHit;
		SafeMoveUpdatedComponent((-WallN * Push), UpdatedComponent->GetComponentQuat(), true, SnapHit);
	}

	if (ACharacter* C = Cast<ACharacter>(PawnOwner))
	{
		const bool bShouldRotateHere = PawnOwner && (PawnOwner->HasAuthority() || PawnOwner->IsLocallyControlled());
		if (!bShouldRotateHere)
		{
			return;
		}

		if (!bIsCeiling)
		{
			FVector FaceDir = FVector::VectorPlaneProject(-WallN, FVector::UpVector).GetSafeNormal();
			if (FaceDir.IsNearlyZero())
			{
				FaceDir = C->GetActorForwardVector();
			}

			const FRotator TargetYaw = FaceDir.Rotation();
			const FRotator NewRot(0.f, TargetYaw.Yaw, 0.f);

			C->SetActorRotation(FMath::RInterpTo(C->GetActorRotation(), NewRot, DeltaTime, RotateInterpSpeed));
		}
		else
		{
			const FVector NewUp = (-WallN).GetSafeNormal();

			FVector Fwd = FVector::VectorPlaneProject(ControlFwd, WallN).GetSafeNormal();
			if (Fwd.IsNearlyZero())
			{
				Fwd = FVector::VectorPlaneProject(C->GetActorForwardVector(), WallN).GetSafeNormal();
			}

			const FRotator TargetRot = FRotationMatrix::MakeFromXZ(Fwd, NewUp).Rotator();
			C->SetActorRotation(FMath::RInterpTo(C->GetActorRotation(), TargetRot, DeltaTime, RotateInterpSpeed));
		}
	}
}

void UGeckoMovementComponent::SetClimbEnabled(bool bEnabled)
{
	bClimbEnabled = bEnabled;

	if (!bClimbEnabled && IsClimbing())
	{
		StopClimb();
	}
}