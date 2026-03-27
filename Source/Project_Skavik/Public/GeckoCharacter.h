// GeckoCharacter.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimInstance.h"
#include "GeckoCharacter.generated.h"

UENUM(BlueprintType)
enum class ESkavikForm : uint8
{
	Base        UMETA(DisplayName = "Base"),
	Velocista   UMETA(DisplayName = "Velocista"),
	Tanque      UMETA(DisplayName = "Tanque"),
	Sanguineo   UMETA(DisplayName = "Sanguineo"),
	Ninja       UMETA(DisplayName = "Ninja")
};

USTRUCT(BlueprintType)
struct FSkavikFormData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	TObjectPtr<USkeletalMesh> Mesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	TSubclassOf<UAnimInstance> AnimClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	float WalkSpeed = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	float SprintSpeed = 750.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	bool bCanClimb = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transformation")
	bool bCanWallRun = false;
};

UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "GeckoCharacter"))
class PROJECT_SKAVIK_API AGeckoCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AGeckoCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Jump() override;

	void MoveForward(float Value);
	void MoveRight(float Value);

	UFUNCTION(BlueprintCallable, Category = "Transformation")
	void OpenTransformationMenu();

	UFUNCTION(BlueprintCallable, Category = "Transformation")
	void CloseTransformationMenu();

	UFUNCTION(BlueprintCallable, Category = "Transformation")
	void ConfirmTransformation(ESkavikForm NewForm);

	UFUNCTION(Server, Reliable)
	void ServerConfirmTransformation(ESkavikForm NewForm);

	UFUNCTION()
	void OnRep_CurrentForm();

	UFUNCTION(BlueprintCallable, Category = "Transformation")
	void ApplyTransformation(ESkavikForm NewForm);

	UFUNCTION(BlueprintCallable, Category = "Transformation")
	FSkavikFormData GetFormData(ESkavikForm Form) const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Transformation")
	void BP_OnTransformationMenuOpened();

	UFUNCTION(BlueprintImplementableEvent, Category = "Transformation")
	void BP_OnTransformationMenuClosed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Transformation")
	void BP_OnFormChanged(ESkavikForm NewForm);

	UFUNCTION(BlueprintCallable, Category = "Climb")
	void ToggleClimb();

	UFUNCTION(Server, Reliable)
	void ServerToggleClimb();

	UFUNCTION(Server, Reliable)
	void ServerClimbJump();

	// WALL RUN
	UFUNCTION(BlueprintCallable, Category = "WallRun")
	void TryWallRun();

	UFUNCTION(Server, Reliable)
	void ServerTryWallRun();

	UFUNCTION(BlueprintPure, Category = "WallRun")
	bool CanCurrentFormWallRun() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "WallRun")
	void BP_TryWallRun();

	UFUNCTION(BlueprintPure, Category = "Transformation")
	ESkavikForm GetCurrentForm() const { return CurrentForm; }

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transformation|Base", meta = (ShowOnlyInnerProperties))
	FSkavikFormData BaseFormData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transformation|Velocista", meta = (ShowOnlyInnerProperties))
	FSkavikFormData VelocistaFormData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transformation|Tanque", meta = (ShowOnlyInnerProperties))
	FSkavikFormData TanqueFormData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transformation|Sanguineo", meta = (ShowOnlyInnerProperties))
	FSkavikFormData SanguineoFormData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transformation|Ninja", meta = (ShowOnlyInnerProperties))
	FSkavikFormData NinjaFormData;

	UFUNCTION(BlueprintCallable, Category = "Transformation")
	ESkavikForm GetFormFromSector(int32 Sector) const;

private:
	float CachedForward = 0.f;
	float CachedRight = 0.f;

	UPROPERTY(EditAnywhere, Category = "Climb|Jump")
	float ClimbJumpOffWallStrength = 600.f;

	UPROPERTY(EditAnywhere, Category = "Climb|Jump")
	float ClimbJumpUpStrength = 450.f;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentForm, EditAnywhere, BlueprintReadOnly, Category = "Transformation|State", meta = (AllowPrivateAccess = "true"))
	ESkavikForm CurrentForm = ESkavikForm::Base;

	UPROPERTY(BlueprintReadOnly, Category = "Transformation", meta = (AllowPrivateAccess = "true"))
	bool bTransformationMenuOpen = false;
};