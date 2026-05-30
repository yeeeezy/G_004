// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h" 
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "Kismet/GameplayStatics.h" 

AFPSCharacter::AFPSCharacter()
{
    PrimaryActorTick.bCanEverTick = true; 

    GetCapsuleComponent()->InitCapsuleSize(40.f, 96.f);

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = false;

    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
    FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
    
    FirstPersonCamera->bUsePawnControlRotation = false; 

    CurrentCameraPitch = 0.0f;

    GravityPlane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GravityPlane"));
    GravityPlane->SetupAttachment(GetMesh()); 
    GravityPlane->SetRelativeLocation(FVector(100.f, 0.f, 50.f)); 

    GravityArrow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GravityArrow"));
    GravityArrow->SetupAttachment(GravityPlane);
    GravityArrow->SetRelativeLocation(FVector(0.f, 0.f, 10.f)); 

    bIsShiftingGravity = false;
    CurrentGyroRotation = FRotator::ZeroRotator;
    AccumulatedPitch = 0.0f;
    AccumulatedRoll = 0.0f;

    bIsSmoothRotating = false;
    TargetActorRotation = FRotator::ZeroRotator;
    SmoothRotationSpeed = 10.0f; 
}

void AFPSCharacter::BeginPlay()
{
    Super::BeginPlay();

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false; 
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->bUseControllerDesiredRotation = false;
}

void AFPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFPSCharacter::Move);
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFPSCharacter::Look);
        if (MouseLookAction)
        {
            EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AFPSCharacter::MouseLook);
        }

        if (GyroDataAction)
        {
            EnhancedInputComponent->BindAction(GyroDataAction, ETriggerEvent::Triggered, this, &AFPSCharacter::InputGyroData);
        }
        if (GyroGravityAction)
        {
            EnhancedInputComponent->BindAction(GyroGravityAction, ETriggerEvent::Started, this, &AFPSCharacter::GyroGravityStarted);
            EnhancedInputComponent->BindAction(GyroGravityAction, ETriggerEvent::Completed, this, &AFPSCharacter::GyroGravityCompleted);
        }
    }
}

void AFPSCharacter::InputGyroData(const FInputActionValue& Value)
{
    FVector GyroVector = Value.Get<FVector>();
    CurrentGyroRotation = FRotator(GyroVector.Y, GyroVector.Z, GyroVector.X); 
}

void AFPSCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsShiftingGravity && GravityPlane)
    {
        // 这里保留了前面的修改：带有负号 (-)，修复重力预瞄的上下反转，并使用了暴露的灵敏度变量
        float PitchDelta = -CurrentGyroRotation.Pitch * DeltaTime * GyroGravitySensitivity;
        float RollDelta  =  CurrentGyroRotation.Roll  * DeltaTime * GyroGravitySensitivity;

        float AbsPitch = FMath::Abs(PitchDelta);
        float AbsRoll  = FMath::Abs(RollDelta);

        float PitchMultiplier = 1.0f; 
        float RollMultiplier  = 1.0f; 

        if (AbsPitch > AbsRoll && AbsPitch > 0.01f)
        {
            AccumulatedPitch += (PitchDelta * PitchMultiplier);
        }
        else if (AbsRoll > AbsPitch && AbsRoll > 0.01f)
        {
            AccumulatedRoll += (RollDelta * RollMultiplier);
        }

        FQuat QPitch(FVector::RightVector, FMath::DegreesToRadians(AccumulatedPitch));
        FQuat QRoll(FVector::ForwardVector, FMath::DegreesToRadians(AccumulatedRoll));
        FQuat NewRelativeRot = InitialPlaneQuat * QPitch * QRoll;

        GravityPlane->SetRelativeRotation(NewRelativeRot);
    }
    else if (bIsSmoothRotating)
    {
        FRotator CurrentActorRot = GetActorRotation();
        FRotator NewActorRot = FMath::RInterpTo(CurrentActorRot, TargetActorRotation, DeltaTime, SmoothRotationSpeed);
        
        SetActorRotation(NewActorRot);
        if (Controller)
        {
            Controller->SetControlRotation(NewActorRot);
        }

        float AngleDifference = FMath::RadiansToDegrees(NewActorRot.Quaternion().AngularDistance(TargetActorRotation.Quaternion()));

        if (AngleDifference < 1.0f)
        {
            SetActorRotation(TargetActorRotation);
            if (Controller)
            {
                Controller->SetControlRotation(TargetActorRotation);
            }

            FVector SnappedUp = TargetActorRotation.Quaternion().GetAxisZ();
            GetCharacterMovement()->SetGravityDirection(-SnappedUp);
            GetCharacterMovement()->SetMovementMode(MOVE_Falling);

            bIsSmoothRotating = false;

            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("【重力姿态对齐完成，已强制 Snap！】"));
            }
        }
    }
}

void AFPSCharacter::GyroGravityStarted()
{
    if (bIsShiftingGravity || bIsSmoothRotating) return;

    bIsShiftingGravity = true;
    AccumulatedPitch = 0.0f;
    AccumulatedRoll = 0.0f;
    
    if (GravityPlane)
    {
        InitialPlaneQuat = GravityPlane->GetRelativeRotation().Quaternion();
    }

    GetCharacterMovement()->Velocity = FVector::ZeroVector;
    GetCharacterMovement()->SetMovementMode(MOVE_None);

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("【重力瞄准模式启动】"));
    }
}

void AFPSCharacter::GyroGravityCompleted()
{
    if (!bIsShiftingGravity) return;

    bIsShiftingGravity = false;

    if (GravityArrow)
    {
        FVector CurrentUp = GravityArrow->GetComponentQuat().GetAxisZ();
        FVector CurrentForward = GravityArrow->GetComponentQuat().GetAxisX();

        FVector SnappedUp = SnapVectorToClosestAxis(CurrentUp);
        FVector ProjectedForward = FVector::VectorPlaneProject(CurrentForward, SnappedUp).GetSafeNormal();
        FVector SnappedForward = SnapVectorToClosestAxis(ProjectedForward);

        FRotator FinalSnappedRot = FRotationMatrix::MakeFromXZ(SnappedForward, SnappedUp).Rotator();
        
        TargetActorRotation = FinalSnappedRot;
        bIsSmoothRotating = true;

        if (GravityPlane)
        {
            GravityPlane->SetRelativeRotation(InitialPlaneQuat);
        }
    }
}

FVector AFPSCharacter::SnapVectorToClosestAxis(const FVector& InVector)
{
    FVector Snapped = FVector::ZeroVector;
    float MaxVal = -1.0f;

    if (FMath::Abs(InVector.X) > MaxVal) { MaxVal = FMath::Abs(InVector.X); Snapped = FVector(FMath::Sign(InVector.X), 0.f, 0.f); }
    if (FMath::Abs(InVector.Y) > MaxVal) { MaxVal = FMath::Abs(InVector.Y); Snapped = FVector(0.f, FMath::Sign(InVector.Y), 0.f); }
    if (FMath::Abs(InVector.Z) > MaxVal) { MaxVal = FMath::Abs(InVector.Z); Snapped = FVector(0.f, 0.f, FMath::Sign(InVector.Z)); }

    return Snapped;
}

void AFPSCharacter::Move(const FInputActionValue& Value)
{
    if (bIsShiftingGravity || bIsSmoothRotating) return;

    FVector2D MovementVector = Value.Get<FVector2D>();
    if (Controller != nullptr)
    {
        AddMovementInput(GetActorForwardVector(), MovementVector.Y);
        AddMovementInput(GetActorRightVector(), MovementVector.X);
    }
}

void AFPSCharacter::Look(const FInputActionValue& Value)
{
    if (bIsShiftingGravity || bIsSmoothRotating) return;

    FVector2D Axis = Value.Get<FVector2D>();
    if (Controller != nullptr)
    {
        float YawInput   = Axis.X * GamepadLookSensitivity * GetWorld()->GetDeltaSeconds();
        
        // ================= 【恢复原状】 =================
        // 恢复为 -Axis.Y，保证平时看四周的习惯是正确的
        float PitchInput = -Axis.Y * GamepadLookSensitivity * GetWorld()->GetDeltaSeconds();

        FQuat YawRotation(FVector::UpVector, FMath::DegreesToRadians(YawInput));
        AddActorLocalRotation(YawRotation);

        CurrentCameraPitch = FMath::Clamp(CurrentCameraPitch + PitchInput, -89.0f, 89.0f);
        FirstPersonCamera->SetRelativeRotation(FRotator(CurrentCameraPitch, 0.f, 0.f));
    }
}

void AFPSCharacter::MouseLook(const FInputActionValue& Value)
{
    if (bIsShiftingGravity || bIsSmoothRotating) return;

    FVector2D Axis = Value.Get<FVector2D>();
    if (Controller != nullptr)
    {
        float YawInput   = Axis.X * MouseLookSensitivity;

        // ================= 【恢复原状】 =================
        // 同样恢复为 -Axis.Y
        float PitchInput = -Axis.Y * MouseLookSensitivity; 

        FQuat YawRotation(FVector::UpVector, FMath::DegreesToRadians(YawInput));
        AddActorLocalRotation(YawRotation);

        CurrentCameraPitch = FMath::Clamp(CurrentCameraPitch + PitchInput, -89.0f, 89.0f);
        FirstPersonCamera->SetRelativeRotation(FRotator(CurrentCameraPitch, 0.f, 0.f));
    }
}