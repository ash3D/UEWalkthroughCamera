#include "UEWalkthroughCameraPrivatePCH.h"
#include "InterpolationPointActor.h"


// Sets default values
AInterpolationPointActor::AInterpolationPointActor() : transform(FObjectInitializer::Get().CreateDefaultSubobject<USceneComponent>(this, TEXT("InterpolationPointTransform")))
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}