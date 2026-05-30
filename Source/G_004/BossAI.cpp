// Fill out your copyright notice in the Description page of Project Settings.

#include "BossAI.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"

ABossAI::ABossAI()
{
    PrimaryActorTick.bCanEverTick = true;

    // ================= 【核心修复 1：防干扰设置】 =================
    // 关闭原生的控制器旋转，我们要用四元数手动控制怪物姿态
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // 关闭 CMC 的自动旋转，因为在多重力下它很容易错乱
    GetCharacterMovement()->bOrientRotationToMovement = false;

    // ================= 【核心修复 2：AI 自动附身】 =================
    // 强制 Boss 无论被摆在场景里还是动态 Spawn 出来，都自动绑定自带的 AI Controller
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    // ================= 【核心修复 3：解决坠落无法移动】 =================
    // 如果重力切换瞬间被判定为在空中，保证它依然能100%响应移动输入，不会卡死在原地
    GetCharacterMovement()->AirControl = 1.0f;
}

void ABossAI::BeginPlay()
{
    Super::BeginPlay();

    // 自动获取玩家作为目标 (索引0通常是本地玩家)
    TargetPlayer = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
}

void ABossAI::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (GetController() == nullptr)
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(1, 0.1f, FColor::Red, TEXT("【警告】Boss 无 Controller！"));
    }

    if (!TargetPlayer) return;

    // 获取玩家的移动组件，读取底层的真实重力
    UCharacterMovementComponent* PlayerCMC = TargetPlayer->FindComponentByClass<UCharacterMovementComponent>();
    if (!PlayerCMC) return;

    // ================= 1. 获取稳定、准确的重力 =================
    // 玩家代码中，重力方向是在对齐完成后瞬间 Snap 的。
    // 我们读取这个绝对稳定的重力方向，而不是读取玩家倾斜过程中的 UpVector
    FVector TargetGravity = PlayerCMC->GetGravityDirection();
    FVector TargetUpVector = -TargetGravity; // 真正的“上方”是重力的反方向

    FVector BossCurrentGravity = GetCharacterMovement()->GetGravityDirection();

    // 如果 Boss 当前的重力跟玩家不一样，说明重力“刚刚”发生了切换！
    if (!BossCurrentGravity.Equals(TargetGravity, 0.01f))
    {
        // 切换 Boss 重力
        GetCharacterMovement()->SetGravityDirection(TargetGravity);
        
        // 【防物理爆炸核心 1】强制进入掉落状态，脱离地面的强制粘附逻辑
        GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        
        // 【防物理爆炸核心 2】顺着 Boss 原本的头顶方向稍微拔高一点点！
        // 这样它的胶囊体在 90 度躺平时，就不会切进地里引发引擎瞬移 (Depenetration)
        AddActorWorldOffset(GetActorUpVector() * 60.0f, false);
    }

    // ================= 2. 状态判定 =================
    // 计算 Boss 当前身体是否已经对齐了新的重力 (夹角是否大于 2 度)
    float AngleDiff = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(GetActorUpVector(), TargetUpVector)));
    bool bIsAligningGravity = (AngleDiff > 2.0f);


    // ================= 3. 更新怪物的 3D 姿态 (四元数旋转) =================
    FVector ToPlayer = TargetPlayer->GetActorLocation() - GetActorLocation();
    FVector MoveDirection = FVector::VectorPlaneProject(ToPlayer, TargetUpVector).GetSafeNormal();

    if (!MoveDirection.IsNearlyZero())
    {
        // 构造目标姿态：Z轴对齐新重力的上方，X轴对准玩家
        FQuat TargetQuat = FRotationMatrix::MakeFromZX(TargetUpVector, MoveDirection).ToQuat();
        
        FQuat NewQuat = FMath::QInterpTo(GetActorQuat(), TargetQuat, DeltaTime, RotationSpeed);
        SetActorRotation(NewQuat);
    }


    // ================= 4. 执行行走逻辑 =================
    // 【防物理爆炸核心 3】只有在姿态基本对齐（转完身了），才允许申请移动！
    if (!bIsAligningGravity)
    {
        float DistanceToPlayer = ToPlayer.Length();
        if (DistanceToPlayer > StopDistance)
        {
            AddMovementInput(MoveDirection, 1.0f);
        }
    }
}