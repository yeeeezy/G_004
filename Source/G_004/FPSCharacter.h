// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FPSCharacter.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class UInputAction;
struct FInputActionValue;

UCLASS()
class AFPSCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AFPSCharacter();

    virtual void Tick(float DeltaTime) override;

protected:
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // ---------------- 【组件】 ----------------
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    UCameraComponent* FirstPersonCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* GravityPlane;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* GravityArrow;

    // ---------------- 【输入动作】 ----------------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* MoveAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* LookAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* JumpAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Gravity")
    UInputAction* GyroGravityAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Gravity")
    UInputAction* GyroDataAction;

    // ---------------- 【核心状态变量】 ----------------
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity|State")
    FRotator CurrentGyroRotation; 

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity|State")
    FQuat InitialPlaneQuat;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity|State")
    bool bIsShiftingGravity;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity|State")
    float AccumulatedPitch;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity|State")
    float AccumulatedRoll;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity|Transition")
    bool bIsSmoothRotating;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity|Transition")
    FRotator TargetActorRotation;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity|Transition")
    float SmoothRotationSpeed;

    // 【新增】：单独记录相机的抬头/低头角度，解决重力改变后的视角死锁问题
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    float CurrentCameraPitch;

    // ---------------- 【内部工具函数】 ----------------
    FVector SnapVectorToClosestAxis(const FVector& InVector);

    // ---------------- 【输入处理函数】 ----------------
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void InputGyroData(const FInputActionValue& Value);
    void GyroGravityStarted();
    void GyroGravityCompleted();
};