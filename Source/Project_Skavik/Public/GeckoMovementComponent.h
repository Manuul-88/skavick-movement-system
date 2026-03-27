// GeckoMovementComponent.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GeckoMovementComponent.generated.h"

UENUM(BlueprintType)
enum ECustomMovementMode : uint8
{
	CMOVE_None     UMETA(DisplayName = "None"),
	CMOVE_Climbing UMETA(DisplayName = "Climbing"),
};

UCLASS() 
class PROJECT_SKAVIK_API UGeckoMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UGeckoMovementComponent();

	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual float GetMaxSpeed() const override;

	UFUNCTION(BlueprintCallable, Category = "Climb")
	void TryStartClimb();

	UFUNCTION(BlueprintCallable, Category = "Climb")
	void StopClimb();

	UFUNCTION(BlueprintPure, Category = "Climb")
	bool IsClimbing() const;

	UFUNCTION(BlueprintPure, Category = "Climb")
	FVector GetCurrentWallNormal() const { return CurrentWallNormal; }

	UFUNCTION(BlueprintCallable, Category = "Sprint")
	void SetSprintRequested(bool bNewSprint);

	UFUNCTION(BlueprintPure, Category = "Sprint")
	bool IsSprintRequested() const { return bSprintRequested; }

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void SetClimbEnabled(bool bEnabled);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climb|Tuning")
	float ClimbSpeed = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climb|Tuning")
	float TraceDistance = 110.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climb|Tuning")
	float SnapStrength = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climb|Tuning")
	float DesiredWallDistance = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climb|Tuning")
	float WallDistanceTolerance = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climb|Tuning")
	float RotateInterpSpeed = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climb|Tuning")
	float ClimbInputDeadzone = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climb|Tuning")
	float NormalInterpSpeed = 18.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climb|Surface")
	bool bAllowCeilingClimb = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climb|Surface", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float CeilingDotThreshold = -0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climb|Surface", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WallDotAbsMax = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprint")
	float WalkSpeed = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprint")
	float SprintSpeed = 750.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climb|Sprint")
	float ClimbSprintMultiplier = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities")
	bool bClimbEnabled = true;

	UPROPERTY(VisibleAnywhere, Category = "Climb|State")
	bool bIsClimbing = false;

	UPROPERTY(VisibleAnywhere, Category = "Climb|State")
	FVector CurrentWallNormal = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, Category = "Climb|State")
	FVector SmoothedWallNormal = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, Category = "Climb|State")
	float LastWallDistance = 0.f;

protected:
	void PhysClimbing(float DeltaTime, int32 Iterations);

	bool TraceWall(FHitResult& OutHit) const;
	bool TraceClimbCandidate(FHitResult& OutHit) const;
	void UpdateWallNormalOrStop();

private:
	bool bSprintRequested = false;
};