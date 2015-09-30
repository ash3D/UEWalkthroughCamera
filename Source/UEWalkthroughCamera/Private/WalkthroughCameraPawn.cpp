#include "UEWalkthroughCameraPrivatePCH.h"
#include "WalkthroughCameraPawn.h"
#include "InterpolationPointActor.h"


DEFINE_LOG_CATEGORY(Walkthrough)

template<class Spline>
class AWalkthroughCameraPawn::CSplineProxy : public ISpline
{
	Spline spline;

public:
	template<typename ...Args>
	CSplineProxy(Args &&...args) : spline(std::forward<Args>(args)...) {}

	virtual TPoint operator ()(TPoint::ElementType u) const override
	{
		return spline(u);
	}
};

// Sets default values
AWalkthroughCameraPawn::AWalkthroughCameraPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	cameraComponent = FObjectInitializer::Get().CreateDefaultSubobject<UCameraComponent>(this, TEXT("WalkthroughCamera"));
	cameraComponent->RelativeLocation = FVector::ZeroVector;
	cameraComponent->bUsePawnControlRotation = false;
}

// Called when the game starts or when spawned
void AWalkthroughCameraPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWalkthroughCameraPawn::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

	if (time <= 1)
	{
		const bool start = time == 0;
		const auto pos = start ? (*spline)(0) : nextPos;

		using namespace Math::Hermite;
		float u = time += DeltaTime * speed;
		if (u < transient)
		{
			u /= transient;
			u = H3(u) + H2(u);
			u *= transient;
		}
		else if (u > 1.f - transient && u < 1.f)
		{
			u -= 1.f - transient;
			u /= transient;
			u = H3(u) + H1(u);
			u *= transient;
			u += 1.f - transient;
		}

		nextPos = (*spline)(u);

		const FMatrix
			swapXY({ 0, 1, 0 }, { 1, 0, 0 }, { 0, 0, 1 }, FVector{ 0 }),
			lookAt(FLookAtMatrix(FVector(pos.x, pos.y, pos.z), FVector(nextPos.x, nextPos.y, nextPos.z), FVector::UpVector));
		FTransform camXform(FTransform(FQuat::MakeFromEuler(FVector(0, -90, 0))).ToMatrixWithScale() * swapXY * lookAt.Inverse());
		const FQuat newRot = camXform.GetRotation();
		const float weightedSmooth = smooth / DeltaTime;
		camXform.SetRotation(curRot = start ? newRot : FMath::Lerp(newRot, curRot, weightedSmooth / (1.f + weightedSmooth)));
		cameraComponent->SetWorldTransform(camXform);

		if (time > 1)
			OnFinish.Broadcast();
	}
}

// Called to bind functionality to input
void AWalkthroughCameraPawn::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	Super::SetupPlayerInputComponent(InputComponent);

}

void AWalkthroughCameraPawn::Run(float speed, float transient, float smooth, bool uniformSpeed)
{
	std::list<std::pair<TPoint, int>> points;
	for (TActorIterator<AInterpolationPointActor> actor(GetWorld()); actor; ++actor)
	{
		const auto pos = actor->GetActorLocation();
		points.emplace_back(std::piecewise_construct, std::make_tuple(pos.X, pos.Y, pos.Z), std::make_tuple(actor->id));
	}
	points.sort([](decltype(points)::const_reference left, decltype(points)::const_reference right)
	{
		return left.second < right.second;
	});

	try
	{
		assert(!points.empty());
		if (points.empty())
			throw TEXT("Trying to start walkthrough with empty path");

		if (points.size() == 1)	// slow!
			points.push_back(points.front());

		auto prevPoint = points.cbegin();	// TODO: use C++14 generalized lambda capture
		const float dist = std::accumulate(std::next(prevPoint), points.cend(), 0, [&prevPoint, uniformSpeed](float curDist, decltype(points)::const_reference point)
		{
			const float offset = Math::VectorMath::distance(prevPoint++->first, point.first);
			if (uniformSpeed)
			{
				assert(offset);
				if (!offset)
					throw TEXT("Dublicating control point in path");
			}
			return curDist + offset;
		});

		if (uniformSpeed)
		{
			assert(dist);
			if (!dist)
				throw TEXT("Trying to start walkthrough with zero length path");
		}

		time = 0;
		this->speed = speed / dist;
		this->transient = fmin(transient * this->speed, .5f);
		this->smooth = smooth;
	}
	catch (const TCHAR error[])
	{
		UE_LOG(Walkthrough, Error, TEXT("%s."), error);
		return;
	}

	points.emplace_front(2 * points.front().first - next(points.cbegin())->first, points.front().second - 1);
	points.emplace_back(2 * points.back().first - prev(points.cend(), 2)->first, points.back().second + 1);

	class CPointIterator : public std::enable_if<true, decltype(points)::const_iterator>::type
	{
		typedef decltype(points)::const_iterator TBase;
#if 0
		decltype(points) &container;
		enum : uint_least8_t
		{
			BEGIN,
			INTERNAL,
			END,
		} location;
#endif

	public:
#if 0
		CPointIterator(decltype(points) &container, bool cbegin) : container(container)/*, TBase(cbegin ? container.cbegin() : container.cend())*/, location(cbegin ? BEGIN : END) {}
#else
		CPointIterator(TBase src) : TBase(src) {}
#endif

	public:
		const TPoint &operator *() const { return TBase::operator *().first; }
		const TPoint *operator ->() const { return &TBase::operator ->()->first; }
		CPointIterator operator ++() { return TBase::operator ++(); }
		CPointIterator operator --() { return TBase::operator --(); }
		CPointIterator operator ++(int) { return TBase::operator ++(0); }
		CPointIterator operator --(int) { return TBase::operator --(0); }
	};

	CPointIterator begin(points.cbegin()), end(points.cend());
	spline = uniformSpeed
		? static_cast<decltype(spline)>(std::make_unique<CSplineProxy<TBesselOverhauser>>	(std::move(begin), std::move(end)))
		: static_cast<decltype(spline)>(std::make_unique<CSplineProxy<TCatmullRom>>			(std::move(begin), std::move(end)));
}

void AWalkthroughCameraPawn::Stop()
{
	time = std::numeric_limits<float>::infinity();
	spline.reset();
}