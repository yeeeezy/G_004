// Copyright Epic Games, Inc. All Rights Reserved.

#include "G_004Character.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h" // 【新增】引入箭头组件头文件
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "G_004.h"

AG_004Character::AG_004Character()
{
    // 【新增】允许角色执行 Tick 函数
    PrimaryActorTick.bCanEverTick = true;

    // Set size for collision capsule
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
       
    // Don't rotate when the controller rotates. Let that just affect the camera.
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // Configure character movement
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
    GetCharacterMovement()->JumpZVelocity = 500.f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxWalkSpeed = 500.f;
    GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
    GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

    // Create a camera boom
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f;
    CameraBoom->bUsePawnControlRotation = true; 

    // Create a follow camera
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    // ---------------- 【新增】初始化重力指示箭头 ----------------
    GravityArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("GravityArrow"));
    GravityArrow->SetupAttachment(RootComponent); 
    GravityArrow->ArrowSize = 3.0f;               
    GravityArrow->SetHiddenInGame(false);         
    
    bIsAimingGravity = false; 
    
    // 初始化陀螺仪记录变量
    CurrentGyroRotation = FRotator::ZeroRotator;
    StartGyroRotation = FRotator::ZeroRotator;
}

void AG_004Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
       
       EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
       EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
       EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AG_004Character::Move);
       EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AG_004Character::Look);
       EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AG_004Character::Look);

       if (GyroDataAction)
       {
           EnhancedInputComponent->BindAction(GyroDataAction, ETriggerEvent::Triggered, this, &AG_004Character::InputGyroData);
       }
       if (GyroGravityAction)
       {
           EnhancedInputComponent->BindAction(GyroGravityAction, ETriggerEvent::Started, this, &AG_004Character::GyroGravityStarted);
           EnhancedInputComponent->BindAction(GyroGravityAction, ETriggerEvent::Completed, this, &AG_004Character::GyroGravityCompleted);
       }
    }
    else
    {
       UE_LOG(LogG_004, Error, TEXT("'%s' Failed to find an Enhanced Input component!"), *GetNameSafe(this));
    }
}

// ====================================================================================
//                             核心逻辑区：保留陀螺仪任意重力计算
// ====================================================================================

void AG_004Character::InputGyroData(const FInputActionValue& Value)
{
    FVector GyroVector = Value.Get<FVector>();
    CurrentGyroRotation = FRotator(GyroVector.Y, GyroVector.Z, GyroVector.X); 
}

void AG_004Character::GyroGravityStarted()
{
    StartGyroRotation = CurrentGyroRotation;
    bIsAimingGravity = true; // 【新增】进入瞄准状态

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("按键已按下！进入重力瞄准模式。"));
    }
}

void AG_004Character::GyroGravityCompleted()
{
    bIsAimingGravity = false; // 【新增】退出瞄准状态

    // 1. 计算相对偏移并规范化
    FRotator DeltaRotation = CurrentGyroRotation - StartGyroRotation;
    DeltaRotation.Normalize(); 

    // 2. 旋转基础重力向量
    FVector DefaultGravityDir(0.0f, 0.0f, -1.0f);
    FVector RawNewGravity = DeltaRotation.RotateVector(DefaultGravityDir);
    FVector SnappedGravity = FVector::ZeroVector;
    float MaxAxisValue = -1.0f;

    // 3. 90度整数吸附算法
    if (FMath::Abs(RawNewGravity.X) > MaxAxisValue) { MaxAxisValue = FMath::Abs(RawNewGravity.X); SnappedGravity = FVector(FMath::Sign(RawNewGravity.X), 0.0f, 0.0f); }
    if (FMath::Abs(RawNewGravity.Y) > MaxAxisValue) { MaxAxisValue = FMath::Abs(RawNewGravity.Y); SnappedGravity = FVector(0.0f, FMath::Sign(RawNewGravity.Y), 0.0f); }
    if (FMath::Abs(RawNewGravity.Z) > MaxAxisValue) { MaxAxisValue = FMath::Abs(RawNewGravity.Z); SnappedGravity = FVector(0.0f, 0.0f, FMath::Sign(RawNewGravity.Z)); }

    // 4. 应用最新的重力方向，让角色贴附墙面
    GetCharacterMovement()->SetGravityDirection(SnappedGravity);

    // 5. 简单粗暴地将摄像机正上方对齐到新的重力反方向
    if (AController* PC = GetController())
    {
        FVector NewUpDirection = -SnappedGravity;
        FVector CurrentCameraForward = PC->GetControlRotation().Vector();
        FRotator NewCameraRotation = FRotationMatrix::MakeFromZX(NewUpDirection, CurrentCameraForward).Rotator();
        PC->SetControlRotation(NewCameraRotation);
    }

    if (GEngine)
    {
        FString DebugMsg = FString::Printf(TEXT("松开！重力已切换: %s"), *SnappedGravity.ToString());
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, DebugMsg);
    }
}

// ====================================================================================
//                             以下恢复为原生默认的移动与视角逻辑
// ====================================================================================

void AG_004Character::Move(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>();
    DoMove(MovementVector.X, MovementVector.Y);
}

void AG_004Character::Look(const FInputActionValue& Value)
{
    FVector2D LookAxisVector = Value.Get<FVector2D>();
    DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AG_004Character::DoMove(float Right, float Forward)
{
    if (GetController() != nullptr)
    {
       const FRotator Rotation = GetController()->GetControlRotation();
       const FRotator YawRotation(0, Rotation.Yaw, 0);

       const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
       const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

       AddMovementInput(ForwardDirection, Forward);
       AddMovementInput(RightDirection, Right);
    }
}

void AG_004Character::DoLook(float Yaw, float Pitch)
{
    if (GetController() != nullptr)
    {
       AddControllerYawInput(Yaw);
       AddControllerPitchInput(Pitch);
    }
}

void AG_004Character::DoJumpStart()
{
    Jump();
}

void AG_004Character::DoJumpEnd()
{
    StopJumping();
}

// ====================================================================================
//                             【新增】实时 Tick 更新箭头
// ====================================================================================

// ====================================================================================
//                             【修改】实时 Tick 更新箭头（指向人物朝向）
// ====================================================================================

void AG_004Character::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsAimingGravity && GravityArrow)
    {
        // === 瞄准模式：实时预览计算 ===
        
        // 1. 依然先算出即将生效的重力方向 (之前的代码)
        FRotator DeltaRotation = CurrentGyroRotation - StartGyroRotation;
        DeltaRotation.Normalize(); 

        FVector DefaultGravityDir(0.0f, 0.0f, -1.0f);
        FVector RawNewGravity = DeltaRotation.RotateVector(DefaultGravityDir);
        FVector SnappedGravity = FVector::ZeroVector;
        float MaxAxisValue = -1.0f;

        if (FMath::Abs(RawNewGravity.X) > MaxAxisValue) { MaxAxisValue = FMath::Abs(RawNewGravity.X); SnappedGravity = FVector(FMath::Sign(RawNewGravity.X), 0.0f, 0.0f); }
        if (FMath::Abs(RawNewGravity.Y) > MaxAxisValue) { MaxAxisValue = FMath::Abs(RawNewGravity.Y); SnappedGravity = FVector(0.0f, FMath::Sign(RawNewGravity.Y), 0.0f); }
        if (FMath::Abs(RawNewGravity.Z) > MaxAxisValue) { MaxAxisValue = FMath::Abs(RawNewGravity.Z); SnappedGravity = FVector(0.0f, 0.0f, FMath::Sign(RawNewGravity.Z)); }

        // 2. 【核心修改】：计算人物未来的正前方
        FVector NewUpDirection = -SnappedGravity;          // 即将变成“天花板”的方向
        FVector CurrentForward = GetActorForwardVector();  // 角色此时此刻的面朝方向

        // 把当前的面朝方向，投影到新的“地面”上，这就得出了角色贴墙后的面朝方向
        FVector PredictedForward = FVector::VectorPlaneProject(CurrentForward, NewUpDirection).GetSafeNormal();

        // 容错处理：如果角色当前的朝向跟新的墙壁完全垂直（即面壁思过状态），投影会失效，此时借用右侧方向
        if (PredictedForward.IsNearlyZero())
        {
            PredictedForward = FVector::VectorPlaneProject(GetActorRightVector(), NewUpDirection).GetSafeNormal();
        }

        // 让箭头指向预测的人物前方
        GravityArrow->SetWorldRotation(PredictedForward.Rotation());
        GravityArrow->SetArrowColor(FColor::Yellow); // 预览颜色
    }
    else if (GravityArrow)
    {
        // === 正常模式：指向当前真实朝向 ===
        
        // 获取角色当前真实的物理朝向
        FVector CurrentForward = GetActorForwardVector();
        GravityArrow->SetWorldRotation(CurrentForward.Rotation());
        GravityArrow->SetArrowColor(FColor::Green); // 锁定颜色
    }
}