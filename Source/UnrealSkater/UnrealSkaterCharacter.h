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
	  // Animation montage variable
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    UAnimMontage* m_pJumpMontage;

    // Gameplay tag container
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayTags")
    FGameplayTagContainer m_gameplayTagsContainer;

	 UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    float m_lerpedVelocityInput;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    FVector2D m_lastInputValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	USoundBase* m_pSkateSoundAsset;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    UAudioComponent* m_pSkateSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    USkeletalMeshComponent* m_pSkateboardSkeletalMesh;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    float m_zDirection;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
    APostProcessVolume* m_pSpeedPostProcess;

    void UpdateLerpedVelocityInput(float DeltaTime);
	void UdpateSkateRotationBasedOnGround(float DeltaTime);
    void AdjustSkateSoundVolumeBaseOnSpeed();
    void UpdateMovement();
    void InitializeSkateSound();
    void SetupSpeedPostProcess();
	void RunSpeedPostProcessEvaluation();
	void HandleJumpSequence();
	void ForceCheckNotOnAir(); 
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

