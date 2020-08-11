// Fill out your copyright notice in the Description page of Project Settings.


#include "Spirit.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "InteractableClasses/InteractableInterface.h"
#include "InteractableClasses/InteractablePlant.h"
#include "Niagara/Public/NiagaraComponent.h"

// Sets default values
ASpirit::ASpirit()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->MaxWalkSpeed = 300;
	GetCharacterMovement()->RotationRate.Yaw = 720;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	// Creates subobject for components
	SkeletalMeshWater = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshWater"));
	StaticMeshIce = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshIce"));
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	NiagaraFootsteps = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraFootsteps"));
	NiagaraRevive = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraRevive"));

	// Setup attachments for components
	SkeletalMeshWater->SetupAttachment(RootComponent);
	StaticMeshIce->SetupAttachment(RootComponent);
	SpringArm->SetupAttachment(RootComponent);
	Camera->SetupAttachment(SpringArm);
	NiagaraFootsteps->SetupAttachment(GetMesh());
	NiagaraRevive->SetupAttachment(GetMesh());

	// Set mesh position offsets
	GetMesh()->SetRelativeLocation(FVector(0, 0, -27));
	SkeletalMeshWater->SetRelativeLocation(FVector(-8, 0, -27));
	StaticMeshIce->SetRelativeLocation(FVector(0, 0, -13));

	// Set character mesh collision profiles
	SkeletalMeshWater->SetCollisionProfileName(TEXT("CharacterMesh"));
	StaticMeshIce->SetCollisionProfileName(TEXT("CharacterMesh"));

	// Set spring arm properties
	SpringArm->TargetArmLength = 300.f;
	SpringArm->bUsePawnControlRotation = true;

	// Set Niagara system properties
	NiagaraRevive->SetRelativeLocation(FVector(0, 0, 25));
	NiagaraRevive->SetRelativeRotation(FRotator(0, 270, 0));
	NiagaraFootsteps->SetAutoActivate(false);
	NiagaraRevive->SetAutoActivate(false);

	// Set capsule base height
	GetCapsuleComponent()->SetCapsuleRadius(12.f);
	GetCapsuleComponent()->SetCapsuleHalfHeight(25.f);

	// Set spirit form to default
	SetForm(ESpiritForm::Default);

	BaseTurnRate = 45.f;
	BaseLookUpAtRate = 45.f;
}

// Called when the game starts or when spawned
void ASpirit::BeginPlay()
{
	Super::BeginPlay();
	GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &ASpirit::OnOverlapBegin);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Destructible, ECR_Block);

}

// Called every frame
void ASpirit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!CanBash) TickBashCooldown(DeltaTime);

	switch (SpiritState)
	{
	case ESpiritState::Idle:
		if (GetVelocity().Size() > 0)
			SetState(ESpiritState::Walking);
		break;
	case ESpiritState::Walking:
		if (GetVelocity().Size() == 0)
			SetState(ESpiritState::Idle);
		if (GetCharacterMovement()->IsFalling())
			SetState(ESpiritState::Falling);
		break;
	case ESpiritState::Falling:
		if (!GetCharacterMovement()->IsFalling())
			SetState(ESpiritState::Idle);
		break;
	case ESpiritState::Squeezing:
		//OnSqueeze();
		break;
	case ESpiritState::Bashing:
		break;
	}
}

// Called to bind functionality to input
void ASpirit::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Binds the action buttons
	PlayerInputComponent->BindAction<SetFormDelegate>("DefaultFormButton", IE_Pressed, this, &ASpirit::SetForm, ESpiritForm::Default);
	PlayerInputComponent->BindAction<SetFormDelegate>("WaterFormButton", IE_Pressed, this, &ASpirit::SetForm, ESpiritForm::Water);
	PlayerInputComponent->BindAction<SetFormDelegate>("IceFormButton", IE_Pressed, this, &ASpirit::SetForm, ESpiritForm::Ice);
	PlayerInputComponent->BindAction("ActionButton", IE_Pressed, this, &ASpirit::Action);

	// Binds the movement axes
	PlayerInputComponent->BindAxis("MoveForward", this, &ASpirit::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASpirit::MoveRight);

	// Binds the camera control axes
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ASpirit::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ASpirit::LookUpAtRate);

}

// Changes the Spirit's form according to input parameter
void ASpirit::SetForm(ESpiritForm DesiredForm)
{
	SpiritForm = DesiredForm;

	switch (SpiritForm)
	{
	case ESpiritForm::Default:
		GetMesh()->SetVisibility(true);
		SkeletalMeshWater->SetVisibility(false);
		StaticMeshIce->SetVisibility(false);

		//GetCapsuleComponent()->SetCapsuleRadius(12.f);
		break;

	case ESpiritForm::Water:
		GetMesh()->SetVisibility(false);
		SkeletalMeshWater->SetVisibility(true);
		StaticMeshIce->SetVisibility(false);

		//GetCapsuleComponent()->SetCapsuleRadius(12.f);
		break;

	case ESpiritForm::Ice:
		GetMesh()->SetVisibility(false);
		SkeletalMeshWater->SetVisibility(false);
		StaticMeshIce->SetVisibility(true);

		//GetCapsuleComponent()->SetCapsuleRadius(15.f);
		break;
	}
}

void ASpirit::MoveForward(float Value)
{
	if (SpiritState != ESpiritState::Squeezing && SpiritState != ESpiritState::Bashing && Controller && Value != 0.f)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ASpirit::MoveRight(float Value)
{
	if (SpiritState != ESpiritState::Squeezing && SpiritState != ESpiritState::Bashing && Controller && Value != 0.f)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Value);
	}
}

void ASpirit::Action()
{
	if (SpiritState != ESpiritState::Falling)
	{
		switch (SpiritForm)
		{
		case ESpiritForm::Default:
			//OnRevive();
			break;
		case ESpiritForm::Water:
			OnJump();
			break;
		case ESpiritForm::Ice:
			if (CanBash) OnBash();
			break;
		}
	}
}

void ASpirit::TickBashCooldown(float DeltaTime)
{
	float BashCooldown = 1.0f;
	static float BashCooldownDuration;

	BashCooldownDuration += DeltaTime;

	if (BashCooldownDuration >= BashCooldown)
	{
		CanBash = true;
		BashCooldownDuration = 0;
	}

	return;
}

void ASpirit::OnRevive_Implementation(AActor* ActorHit)
{
	UE_LOG(LogTemp, Warning, TEXT("HELLO"));
	if (ActorHit)
	{
		AInteractablePlant* InteractablePlant = Cast<AInteractablePlant>(ActorHit);
		//IInteractInterface* Interface = Cast<IInteractInterface>(ActorHit);
		if (!InteractablePlant->bPlantIsGrown)
		{
			IInteractableInterface* Interface = Cast<IInteractableInterface>(ActorHit);
			if (Interface)
				Interface->Execute_OnInteract(ActorHit, this);
		}
		else
		{
			IInteractableInterface* Interface = Cast<IInteractableInterface>(ActorHit);
			if (Interface)
				Interface->Execute_OnInteractEnd(ActorHit, this);
		}
	}
}

void ASpirit::OnBash_Implementation()
{
	float BashDuration = 0.15f;
	static float BashTime;

	CanBash = false;
	GetCapsuleComponent()->SetCapsuleRadius(20.f);
	UE_LOG(LogTemp, Warning, TEXT("CapsuleRadius: %f"), GetCapsuleComponent()->GetScaledCapsuleRadius());
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Destructible, ECR_Overlap);
	FRotator SpiritRotation = GetCapsuleComponent()->GetComponentRotation();
	const FVector Direction = FRotationMatrix(SpiritRotation).GetUnitAxis(EAxis::X);

	// TODO Character only breaks debris when bashing right next to it because the collision channel change is only activated
	// at the moment the bash begins
	// Only works because the collision size increases on launch then switches back after
	ACharacter::LaunchCharacter(FVector(Direction.X, Direction.Y, 0.f).GetSafeNormal() * BashDistance, true, true);
	GetCapsuleComponent()->SetCapsuleRadius(12.f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Destructible, ECR_Block);

	//if (BashTime < BashDuration)
	//{
	//	FRotator SpiritRotation = GetCapsuleComponent()->GetComponentRotation();
	//	const FVector Direction = FRotationMatrix(SpiritRotation).GetUnitAxis(EAxis::X);
	//	ACharacter::LaunchCharacter(FVector(Direction.X, Direction.Y, 0.f).GetSafeNormal() * BashDistance, true, true);
	//	BashTime += GetWorld()->GetDeltaSeconds();
	//}
	//else
	//{
	//	BashTime = 0.f;
	//	//CanBash = false;
	//	//SphereComponent->SetCollisionResponseToChannel(ECC_Destructible, ECR_Block);
	//	SetState(ESpiritState::Idle);
	//}
}

void ASpirit::OnSqueeze_Implementation(AActor* ActorHit)
{
	if (ActorHit)
	{
		IInteractableInterface* Interface = Cast<IInteractableInterface>(ActorHit);
		if (Interface)
			Interface->Execute_OnInteract(ActorHit, this);
	}
}

void ASpirit::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("Overlap called"));
	IInteractableInterface* Interface = Cast<IInteractableInterface>(OtherActor);
	if (Interface)
		Interface->Execute_OnInteract(OtherActor, this);
}
