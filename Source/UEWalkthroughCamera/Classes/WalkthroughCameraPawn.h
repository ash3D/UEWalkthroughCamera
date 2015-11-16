#pragma once

#include <memory>
#include <limits>
#include "GameFramework/Pawn.h"
#include "WalkthroughCameraPawn.generated.h"

#if defined _MSC_VER && _MSC_VER < 1900
#error old compiler version
#endif

DECLARE_LOG_CATEGORY_EXTERN(Walkthrough, Log, All);

UCLASS(meta = (ShortTooltip = "This camera will follow spline curve defined by sequence of points represented by InterpolationPointActor."))
class AWalkthroughCameraPawn : public APawn
{
	GENERATED_BODY()

	class UCameraComponent *cameraComponent;

public:
	// Sets default values for this pawn's properties
	AWalkthroughCameraPawn();
	~AWalkthroughCameraPawn();

	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

public:
	UFUNCTION(BlueprintCallable, Category = "Walhthrough")
	void Run(float speed, float transient, bool uniformSpeed);

	UFUNCTION(BlueprintCallable, Category = "Walhthrough")
	void Stop();

private:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFinishDelegate);
	UPROPERTY(BlueprintAssignable, Category = "Walhthrough")
	FFinishDelegate OnFinish;

private:
	/*
		std::unique_ptr does not work with forward decl here despite external dtor have been defined (but it works fine in syntetic test outside of UE4).
		The reason is propably UE4's autogenerated ctor (VTABLE_PTR_HELPER_CTOR in particular) which causes reference to std::unique_ptr's dtor (due to possible exception in ctor).
		TSplineDeleter with external 'operator ()' used as a workaround.
		TODO: track changes in UE4 which can affect this situation.
	*/
	struct ISpline;
	template<class Spline> class CSplineProxy;

	struct TSplineDeleter
	{
		void operator ()(ISpline *spline) const;
	};
	std::unique_ptr<ISpline, TSplineDeleter> spline;

	float time = std::numeric_limits<float>::infinity(), speed, transient;
};