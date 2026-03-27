// GeckoCharacter.cpp
#include "GeckoCharacter.h"
#include "GeckoMovementComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"

AGeckoCharacter::AGeckoCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UGeckoMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGeckoCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGeckoCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGeckoCharacter::MoveRight);

	PlayerInputComponent->BindAction("Climb", IE_Pressed, this, &AGeckoCharacter::ToggleClimb);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AGeckoCharacter::Jump);

	// Si quieres también mapear WallRun desde C++ con un Action Mapping separado, descomenta esto:
	// PlayerInputComponent->BindAction("WallRun", IE_Pressed, this, &AGeckoCharacter::TryWallRun);
}

void AGeckoCharacter::MoveForward(float Value)
{
	CachedForward = Value;

	if (Controller && FMath::Abs(Value) > KINDA_SMALL_NUMBER)
	{
		const FRotator YawRot(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Dir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
		AddMovementInput(Dir, Value);
	}
}

void AGeckoCharacter::MoveRight(float Value)
{
	CachedRight = Value;

	if (Controller && FMath::Abs(Value) > KINDA_SMALL_NUMBER)
	{
		const FRotator YawRot(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Dir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
		AddMovementInput(Dir, Value);
	}
}

void AGeckoCharacter::ToggleClimb()
{
	if (HasAuthority())
	{
		if (UGeckoMovementComponent* GM = Cast<UGeckoMovementComponent>(GetCharacterMovement()))
		{
			if (GM->IsClimbing())
			{
				GM->StopClimb();
			}
			else
			{
				GM->TryStartClimb();
			}
		}
		return;
	}

	ServerToggleClimb();
}

void AGeckoCharacter::ServerToggleClimb_Implementation()
{
	if (UGeckoMovementComponent* GM = Cast<UGeckoMovementComponent>(GetCharacterMovement()))
	{
		if (GM->IsClimbing())
		{
			GM->StopClimb();
		}
		else
		{
			GM->TryStartClimb();
		}
	}
}

void AGeckoCharacter::TryWallRun()
{
	if (!CanCurrentFormWallRun())
	{
		return;
	}

	// Cliente local: dispara feedback/local BP y avisa al server
	if (IsLocallyControlled() && !HasAuthority())
	{
		BP_TryWallRun();
		ServerTryWallRun();
		return;
	}

	// Server o standalone
	BP_TryWallRun();
}

void AGeckoCharacter::ServerTryWallRun_Implementation()
{
	if (!CanCurrentFormWallRun())
	{
		return;
	}

	BP_TryWallRun();
}

bool AGeckoCharacter::CanCurrentFormWallRun() const
{
	const FSkavikFormData FormData = GetFormData(CurrentForm);
	return FormData.bCanWallRun;
}

void AGeckoCharacter::Jump()
{
	if (UGeckoMovementComponent* GM = Cast<UGeckoMovementComponent>(GetCharacterMovement()))
	{
		if (GM->IsClimbing())
		{
			const FVector WallN = GM->GetCurrentWallNormal().GetSafeNormal();
			const FVector LaunchVel =
				(-WallN * ClimbJumpOffWallStrength) +
				(FVector::UpVector * ClimbJumpUpStrength);

			if (IsLocallyControlled() && !HasAuthority())
			{
				GM->StopClimb();
				LaunchCharacter(LaunchVel, true, true);
				ServerClimbJump();
				return;
			}

			GM->StopClimb();
			LaunchCharacter(LaunchVel, true, true);
			return;
		}
	}

	Super::Jump();
}

void AGeckoCharacter::ServerClimbJump_Implementation()
{
	if (UGeckoMovementComponent* GM = Cast<UGeckoMovementComponent>(GetCharacterMovement()))
	{
		if (GM->IsClimbing())
		{
			const FVector WallN = GM->GetCurrentWallNormal().GetSafeNormal();
			const FVector LaunchVel =
				(-WallN * ClimbJumpOffWallStrength) +
				(FVector::UpVector * ClimbJumpUpStrength);

			GM->StopClimb();
			LaunchCharacter(LaunchVel, true, true);
		}
	}
}

void AGeckoCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGeckoCharacter, CurrentForm);
}

void AGeckoCharacter::OpenTransformationMenu()
{
	if (!IsLocallyControlled())
	{
		return;
	}

	bTransformationMenuOpen = true;

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = true;

		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
	}

	BP_OnTransformationMenuOpened();
}

void AGeckoCharacter::CloseTransformationMenu()
{
	if (!IsLocallyControlled())
	{
		return;
	}

	bTransformationMenuOpen = false;

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = false;

		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
	}

	BP_OnTransformationMenuClosed();
}

void AGeckoCharacter::ConfirmTransformation(ESkavikForm NewForm)
{
	CloseTransformationMenu();

	if (HasAuthority())
	{
		CurrentForm = NewForm;
		ApplyTransformation(NewForm);
		BP_OnFormChanged(CurrentForm);
		return;
	}

	ServerConfirmTransformation(NewForm);
}

void AGeckoCharacter::ServerConfirmTransformation_Implementation(ESkavikForm NewForm)
{
	CurrentForm = NewForm;
	ApplyTransformation(NewForm);
	BP_OnFormChanged(CurrentForm);
}

void AGeckoCharacter::OnRep_CurrentForm()
{
	ApplyTransformation(CurrentForm);
	BP_OnFormChanged(CurrentForm);
}

FSkavikFormData AGeckoCharacter::GetFormData(ESkavikForm Form) const
{
	switch (Form)
	{
	case ESkavikForm::Base:
		return BaseFormData;

	case ESkavikForm::Velocista:
		return VelocistaFormData;

	case ESkavikForm::Tanque:
		return TanqueFormData;

	case ESkavikForm::Sanguineo:
		return SanguineoFormData;

	case ESkavikForm::Ninja:
		return NinjaFormData;

	default:
		return BaseFormData;
	}
}

void AGeckoCharacter::ApplyTransformation(ESkavikForm NewForm)
{
	CurrentForm = NewForm;

	const FSkavikFormData FormData = GetFormData(NewForm);

	if (GetMesh() && FormData.Mesh)
	{
		GetMesh()->SetSkeletalMesh(FormData.Mesh);
	}

	if (GetMesh() && FormData.AnimClass)
	{
		GetMesh()->SetAnimInstanceClass(FormData.AnimClass);
	}

	if (UGeckoMovementComponent* GM = Cast<UGeckoMovementComponent>(GetCharacterMovement()))
	{
		GM->WalkSpeed = FormData.WalkSpeed;
		GM->SprintSpeed = FormData.SprintSpeed;
		GM->SetClimbEnabled(FormData.bCanClimb);
	}
}

ESkavikForm AGeckoCharacter::GetFormFromSector(int32 Sector) const
{
	switch (Sector)
	{
	case 0:
		return ESkavikForm::Sanguineo;

	case 1:
		return ESkavikForm::Base;

	case 2:
		return ESkavikForm::Ninja;

	case 3:
		return ESkavikForm::Velocista;

	case 4:
		return ESkavikForm::Tanque;

	default:
		return ESkavikForm::Base;
	}
}