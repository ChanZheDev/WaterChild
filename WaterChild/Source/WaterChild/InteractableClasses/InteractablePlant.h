// Copyright SkyJus Works. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interactable.h"
#include "InteractablePlant.generated.h"

UENUM(BlueprintType)
enum class EPlantState : uint8
{
	Dead		UMETA(DisplayName = "Dead"),
	Reviving	UMETA(DisplayName = "Reviving"),
	Revived		UMETA(DisplayName = "Revived"),
};

UCLASS()
class WATERCHILD_API AInteractablePlant : public AInteractable
{
	GENERATED_BODY()
	
public:
	AInteractablePlant();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void OnInteract_Implementation(ASpirit* Caller) override;
	virtual void OnInteractEnd_Implementation(ASpirit* Caller) override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* FloorPlane;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* StemMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh", meta = (AllowPrivateAccess = "true"))
	class USkeletalMeshComponent* PetalMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collider", meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* PetalCollider;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collider", meta = (AllowPrivateAccess = "true"))
	class UBoxComponent* ReviveCollider;

	EPlantState PlantState = EPlantState::Dead;
	EPlantState GetPlantState() { return PlantState; }
	void SetPlantState(EPlantState DesiredState) { PlantState = DesiredState; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Revive Settings", meta = (AllowPrivateAccess = "true"))
	float ReviveSpeed = 100;
	float DefaultPlantHeight = -372;
	float RevivedPlantHeight = 0;
};
