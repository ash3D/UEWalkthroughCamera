#pragma once

#include "GameFramework/Actor.h"
#include "InterpolationPointActor.generated.h"

UCLASS(meta = (ShortTooltip = "Defines point for WalkthroughCameraPawn animation curve. Member \"id\" determines order."))
class AInterpolationPointActor : public AActor
{
	GENERATED_BODY()

	class USceneComponent * /*const*/ transform;
	
public:	
	// Sets default values for this actor's properties
	AInterpolationPointActor();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 id;
};