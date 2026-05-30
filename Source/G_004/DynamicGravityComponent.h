#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DynamicGravityComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class G_004_API UDynamicGravityComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UDynamicGravityComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

private:
	// 缓存挂载此组件的物理网格体
	class UPrimitiveComponent* PhysicsComp;
    
	// 缓存玩家角色，用来读取重力方向
	class ACharacter* PlayerCharacter;
	
	FVector LastGravityDir = FVector::ZeroVector;
};