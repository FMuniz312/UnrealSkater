// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnrealSkaterCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"   
#include "Sound/SoundCue.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/AudioComponent.h"  
#include "Kismet/GameplayStatics.h"  

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AUnrealSkaterCharacter


AUnrealSkaterCharacter::AUnrealSkaterCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; 
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); 


	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	m_pCameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	m_pCameraBoom->SetupAttachment(RootComponent);
	m_pCameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	m_pCameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	m_pFollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	m_pFollowCamera->SetupAttachment(m_pCameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	m_pFollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Create a skeletal mesh for the skate
	m_pSkateboardSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("m_pSkateboardSkeletalMesh"));
	m_pSkateboardSkeletalMesh->SetupAttachment(GetMesh()); // Attach the skate mesh to the character's mesh at a specified socket

}

void AUnrealSkaterCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
	InitializeSkateSound();
}

void AUnrealSkaterCharacter::Tick(float DeltaTime)
{
	UdpateSkateRotationBasedOnGround(DeltaTime);
	UpdateLerpedVelocityInput(DeltaTime);
	if(m_pSkateSound && m_pSkateSound->IsActive())m_pSkateSound->AdjustVolume(0.1f, m_lerpedVelocityInput, EAudioFaderCurve::Linear);
	UpdateMovement();
	RunSpeedPostProcessEvaluation();
	ForceCheckNotOnAir();
}

//////////////////////////////////////////////////////////////////////////
// Input

void AUnrealSkaterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AUnrealSkaterCharacter::HandleJumpSequence);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Canceled, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AUnrealSkaterCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AUnrealSkaterCharacter::Look);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AUnrealSkaterCharacter::Move(const FInputActionValue& Value)
{
	// Store the last input value
	m_lastInputValue = Value.Get<FVector2D>();
}

void AUnrealSkaterCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}
void AUnrealSkaterCharacter::UpdateLerpedVelocityInput(float DeltaTime)
{
	float lastInputValueZClampled = UKismetMathLibrary::FClamp(m_lastInputValue.Y, -1.0, 1.0);
	if (UKismetMathLibrary::NearlyEqual_FloatFloat(lastInputValueZClampled, m_lerpedVelocityInput, 0.000001))
	{
		m_lerpedVelocityInput = UKismetMathLibrary::FClamp(lastInputValueZClampled,0,1);
	}
	else 
	{
		float interpValue = UKismetMathLibrary::FInterpTo(m_lerpedVelocityInput,lastInputValueZClampled, DeltaTime, 1.0);

		m_lerpedVelocityInput = UKismetMathLibrary::FClamp(interpValue, 0, 1);

	}
	 
	
}
void AUnrealSkaterCharacter::UdpateSkateRotationBasedOnGround(float DeltaTime)
{
	FVector frontWheelLocation = m_pSkateboardSkeletalMesh->GetSocketLocation(TEXT("SKT_FrontWheels"));
	FVector backWheelLocation = m_pSkateboardSkeletalMesh->GetSocketLocation(TEXT("SKT_BackWheels"));

	FVector frontHitLocation;
	FVector backHitLocation;

	RaycastWheelsToGround(frontWheelLocation, frontHitLocation);
	RaycastWheelsToGround(backWheelLocation, backHitLocation);

	FVector lookAtTarget = (frontHitLocation + backHitLocation) / 2;
	FRotator targetRotation = UKismetMathLibrary::FindLookAtRotation(backHitLocation, frontHitLocation);

	FRotator currentRotation = m_pSkateboardSkeletalMesh->GetComponentRotation();
	FRotator interpRotation = UKismetMathLibrary::RInterpTo(currentRotation, targetRotation, DeltaTime, 10.0f);

	FRotator finalRotation;
	finalRotation.Roll = currentRotation.Roll;
	finalRotation.Pitch = interpRotation.Pitch;
	finalRotation.Yaw = currentRotation.Yaw;
	m_pSkateboardSkeletalMesh->SetWorldRotation(finalRotation);

}
void AUnrealSkaterCharacter::AdjustSkateSoundVolumeBaseOnSpeed()
{
	if (IsValid(m_pSkateSound))
	{
		float AdjustVolumeLevel = FMath::Clamp(m_lerpedVelocityInput, 0.0f, 1.0f);
		m_pSkateSound->AdjustVolume(0.1f, AdjustVolumeLevel, EAudioFaderCurve::Linear);
	}
}

void AUnrealSkaterCharacter::UpdateMovement()
{
	m_zDirection = m_pSkateboardSkeletalMesh->GetForwardVector().Z;	

	float mappedZDirection = UKismetMathLibrary::MapRangeClamped(m_zDirection, -1.0, 1.0, -0.7, 0.7);
	double inputValueY = m_lerpedVelocityInput - mappedZDirection;

	double inputValueX = m_lastInputValue.X * 0.05;

	AddMovementInput(m_pSkateboardSkeletalMesh->GetForwardVector(), inputValueY, false);
	AddMovementInput(m_pSkateboardSkeletalMesh->GetRightVector(), inputValueX, false);

}

void AUnrealSkaterCharacter::InitializeSkateSound()
{
	if (m_pSkateSoundAsset)
	{
		m_pSkateSound = UGameplayStatics::CreateSound2D(this, m_pSkateSoundAsset, 0.4f, 1.0f, 0.0f, nullptr, false, false);
		if (m_pSkateSound)
		{
			m_pSkateSound->Play();

			m_pSkateSound->AdjustVolume(0.1f, 0, EAudioFaderCurve::Linear);
		}
	}
}

void AUnrealSkaterCharacter::SetupSpeedPostProcess()
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), APostProcessVolume::StaticClass(), FName("SkaterPP"), FoundActors);

	if (FoundActors.Num() > 0)
	{
		m_pSpeedPostProcess = Cast<APostProcessVolume>(FoundActors[0]);
	}
}

void AUnrealSkaterCharacter::RunSpeedPostProcessEvaluation()
{
	if (m_pSpeedPostProcess)
	{
		m_pSpeedPostProcess->BlendWeight = m_lerpedVelocityInput;

		float InFieldOfView = UKismetMathLibrary::MapRangeClamped(m_lerpedVelocityInput, 0.5, 1.0, 90.0, 120.0);
		m_pFollowCamera->SetFieldOfView(InFieldOfView);
	}
}

void AUnrealSkaterCharacter::HandleJumpSequence()
{
	Jump();

	FGameplayTag JumpTag = FGameplayTag::RequestGameplayTag(FName("Character.Actions.Jumping"));
	m_gameplayTagsContainer.AddTag(JumpTag);

	if (m_pJumpMontage && GetMesh())
	{
		 
 		PlayAnimMontage(m_pJumpMontage, 1.0f, NAME_None);

 		FVector Impulse = m_pSkateboardSkeletalMesh->GetForwardVector() * 500.0f;
		GetCharacterMovement()->AddImpulse(Impulse, true);
	}


}

void AUnrealSkaterCharacter::ForceCheckNotOnAir()
{
	if (FindComponentByClass<UCharacterMovementComponent>() && !FindComponentByClass<UCharacterMovementComponent>()->IsFalling())
	{
		FGameplayTag JumpingTag = FGameplayTag::RequestGameplayTag(FName("Character.Actions.Jumping"));

		if (m_gameplayTagsContainer.HasTagExact(JumpingTag))
		{			
			m_gameplayTagsContainer.RemoveTag(JumpingTag);
		}
	}
}

void AUnrealSkaterCharacter::RaycastWheelsToGround(FVector WheelLocation, FVector& OutHitLocation)
{
	FVector Start = WheelLocation + FVector(0.0f, 0.0f, 50.0f);
	FVector End = WheelLocation + FVector(0.0f, 0.0f, -50.0f);

	FHitResult HitResult;
	bool bHit = UKismetSystemLibrary::LineTraceSingle(
		this,
		Start,
		End,
		ETraceTypeQuery::TraceTypeQuery2,
		false,
		TArray<AActor*>(),
		EDrawDebugTrace::None,
		HitResult,
		true,
		FLinearColor::Red,
		FLinearColor::Green,
		5.0f
	);

	if (bHit)
	{
		OutHitLocation = HitResult.Location;
	}
	else
	{
		OutHitLocation = FVector(0.0f, 0.0f, 0.0f);
	}
}
