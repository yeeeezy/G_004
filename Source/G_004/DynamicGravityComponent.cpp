#include "DynamicGravityComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UDynamicGravityComponent::UDynamicGravityComponent()
{
	// 允许该组件每帧 Tick
	PrimaryComponentTick.bCanEverTick = true;
}

void UDynamicGravityComponent::BeginPlay()
{
	Super::BeginPlay();

	// 1. 获取这个组件挂载的物体的根组件（通常就是那个模拟物理的 Mesh）
	PhysicsComp = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());
    
	if (PhysicsComp && PhysicsComp->IsSimulatingPhysics())
	{
		// 【核心操作】：关闭引擎原生向下的重力
		PhysicsComp->SetEnableGravity(false);
	}

	// 2. 缓存场景里的玩家，方便每帧读取玩家的重力方向
	PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
}

void UDynamicGravityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (PhysicsComp && PhysicsComp->IsSimulatingPhysics() && PlayerCharacter)
	{
		FVector CurrentGravityDir = PlayerCharacter->GetCharacterMovement()->GetGravityDirection();

		// 1. 【防闪现核心】：如果发现重力方向变了，立刻用一巴掌把箱子“扇醒”！
		// 防止它在休眠状态下被玩家翻滚的胶囊体卡住导致物理爆炸
		if (!CurrentGravityDir.Equals(LastGravityDir, 0.01f))
		{
			PhysicsComp->WakeRigidBody();
			LastGravityDir = CurrentGravityDir;
		}

		// 2. 【物理稳定核心】：使用 bAccelChange = true
		// 注意 AddForce 的第三个参数变成了 true。
		// 这意味着 980.0f 不再是力，而是“重力加速度”。
		// 它会完全模拟引擎真实的重力感，不管箱子是一克还是一吨，都能完美匀加速下落，大大减少鬼畜。
		PhysicsComp->AddForce(CurrentGravityDir * 980.0f, NAME_None, true); 
	}
}