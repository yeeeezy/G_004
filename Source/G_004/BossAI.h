// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BossAI.generated.h"

UCLASS()
class G_004_API ABossAI : public ACharacter
{
	GENERATED_BODY()

public:
	ABossAI();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

public:
	// --- AI 属性 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss AI")
	float StopDistance = 150.0f;  // 靠近玩家多近时停止移动

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss AI")
	float RotationSpeed = 8.0f;   // 转身和适应新地面的平滑度

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss AI")
	float FloorTraceDistance = 500.0f; // 向下探测地面的射线长度

private:
	// 缓存目标玩家的指针
	AActor* TargetPlayer;
};