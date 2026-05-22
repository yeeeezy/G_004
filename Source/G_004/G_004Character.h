// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "G_004Character.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class UArrowComponent; // 【新增】前置声明箭头组件

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(abstract)
class AG_004Character : public ACharacter
{
    GENERATED_BODY()

    /** Camera boom positioning the camera behind the character */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
    USpringArmComponent* CameraBoom;

    /** Follow camera */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
    UCameraComponent* FollowCamera;

protected:

    // ---------------- 原有基础输入 ----------------
    UPROPERTY(EditAnywhere, Category="Input")
    UInputAction* JumpAction;

    UPROPERTY(EditAnywhere, Category="Input")
    UInputAction* MoveAction;

    UPROPERTY(EditAnywhere, Category="Input")
    UInputAction* LookAction;

    UPROPERTY(EditAnywhere, Category="Input")
    UInputAction* MouseLookAction;

    // ---------------- 陀螺仪与重力输入 ----------------
    UPROPERTY(EditAnywhere, Category="Input|Gyro")
    UInputAction* GyroGravityAction;

    UPROPERTY(EditAnywhere, Category="Input|Gyro")
    UInputAction* GyroDataAction;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gyro")
    FRotator CurrentGyroRotation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gyro")
    FRotator StartGyroRotation;

    // ---------------- 【新增】箭头组件与状态 ----------------
    /** 用于显示重力方向的箭头 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    UArrowComponent* GravityArrow;

    /** 标识玩家当前是否正在按住按键瞄准重力 */
    bool bIsAimingGravity;

public:
    AG_004Character(); 

    // 【新增】允许每帧更新以刷新箭头方向
    virtual void Tick(float DeltaTime) override;

protected:
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    
    void InputGyroData(const FInputActionValue& Value);
    void GyroGravityStarted();
    void GyroGravityCompleted();

public:

    UFUNCTION(BlueprintCallable, Category="Input")
    virtual void DoMove(float Right, float Forward);

    UFUNCTION(BlueprintCallable, Category="Input")
    virtual void DoLook(float Yaw, float Pitch);

    UFUNCTION(BlueprintCallable, Category="Input")
    virtual void DoJumpStart();

    UFUNCTION(BlueprintCallable, Category="Input")
    virtual void DoJumpEnd();

    FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
    FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};