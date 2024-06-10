// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/PostProcessVolume.h"
#include "GameplayTagContainer.h"
#include "UnrealSkaterCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class AUnrealSkaterCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* m_pCameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* m_pFollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

protected:
// Animation montage variable for handling jump animations
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
UAnimMontage* m_pJumpMontage;

// Container for managing gameplay tags
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayTags")
FGameplayTagContainer m_gameplayTagsContainer;

// Lerp value for character velocity, read-only in Blueprints
UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
float m_lerpedVelocityInput;

// Last input value for character movement, read-only in Blueprints
UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
FVector2D m_lastInputValue;

// Sound asset for skate sound, editable and writable in Blueprints
UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
USoundBase* m_pSkateSoundAsset;

// Audio component for skate sound, read-only in Blueprints
UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
UAudioComponent* m_pSkateSound;

// Skeletal mesh component for the skateboard, editable and writable in Blueprints
UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
USkeletalMeshComponent* m_pSkateboardSkeletalMesh;

// Directional value for z-axis, read-only in Blueprints
UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
float m_zDirection;

// Post-process volume for speed effects, editable and writable in Blueprints
UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
APostProcessVolume* m_pSpeedPostProcess;

// Updates lerped velocity input based on DeltaTime
void UpdateLerpedVelocityInput(float DeltaTime);

// Updates skate rotation based on ground, taking DeltaTime as input
void UdpateSkateRotationBasedOnGround(float DeltaTime);

// Adjusts the skate sound volume based on speed
void AdjustSkateSoundVolumeBaseOnSpeed();

// Main function to cache the character's intended movement
void UpdateMovement();

// Initializes the skateboarding sound
void InitializeSkateSound();

// Setup the speed post-process volume
void SetupSpeedPostProcess();

// Evaluates and run the speed post-process effects
void RunSpeedPostProcessEvaluation();

// Handles the jump sequence for the character
void HandleJumpSequence();

// Checks if the character is not in the air and enforce it
void ForceCheckNotOnAir();

// Performs a raycast from the wheels to the ground
void RaycastWheelsToGround(FVector WheelLocation, FVector& OutHitLocation);

public:
	AUnrealSkaterCharacter();
	

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);
			
 
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay();
	virtual void Tick(float DeltaTime) override;
	
	 

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return m_pCameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return m_pFollowCamera; }
};

