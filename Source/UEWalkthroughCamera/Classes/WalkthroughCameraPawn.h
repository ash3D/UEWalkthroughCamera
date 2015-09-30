#pragma once

#include <memory>
#include <limits>
#include <type_traits>
#include "splines.h"
#include "GameFramework/Pawn.h"
#include "WalkthroughCameraPawn.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(Walkthrough, Log, All);

UCLASS(meta = (ShortTooltip = "This camera will follow spline curve defined by sequence of points represented by InterpolationPointActor."))
class AWalkthroughCameraPawn : public APawn
{
	GENERATED_BODY()

	class UCameraComponent *cameraComponent;

public:
	// Sets default values for this pawn's properties
	AWalkthroughCameraPawn();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

public:
	UFUNCTION(BlueprintCallable, Category = "Walhthrough")
	void Run(float speed, float transient, float smooth, bool uniformSpeed);

	UFUNCTION(BlueprintCallable, Category = "Walhthrough")
	void Stop();

private:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFinishDelegate);
	UPROPERTY(BlueprintAssignable, Category = "Walhthrough")
	FFinishDelegate OnFinish;

private:
	typedef Math::Splines::CCatmullRom<float, 3> TCatmullRom;
	typedef Math::Splines::CBesselOverhauser<float, 3> TBesselOverhauser;
	typedef std::common_type<TCatmullRom::TPoint, TBesselOverhauser::TPoint>::type TPoint;
	struct ISpline
	{
		virtual TPoint operator ()(TPoint::ElementType u) const = 0;

	public:
		virtual ~ISpline() {}
	};
	template<class Spline> class CSplineProxy;

	std::unique_ptr<ISpline> spline;
	TPoint nextPos;
	FQuat curRot;
	float time = std::numeric_limits<float>::infinity(), speed, transient, smooth;
};