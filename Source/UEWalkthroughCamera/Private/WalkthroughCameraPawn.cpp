#include "UEWalkthroughCameraPrivatePCH.h"
#include "WalkthroughCameraPawn.h"
#include "InterpolationPointActor.h"


DEFINE_LOG_CATEGORY(Walkthrough)

using namespace Math::VectorMath::HLSL;

typedef Math::Splines::CCatmullRom<float, 3, float3> TCatmullRom;
typedef Math::Splines::CBesselOverhauser<float, 3, float3> TBesselOverhauser;
typedef std::common_type<TCatmullRom::Point, TBesselOverhauser::Point>::type Point;

struct AWalkthroughCameraPawn::ISpline
{
	virtual ~ISpline() = default;
	virtual Point operator ()(decltype(Point::pos)::ElementType u) const = 0;
};

template<class Spline>
class AWalkthroughCameraPawn::CSplineProxy final : public ISpline
{
	const Spline spline;

public:
	template<typename ...Args>
	CSplineProxy(Args &&...args) : spline(std::forward<Args>(args)...) {}

private:
	virtual Point operator ()(typename std::enable_if<true, decltype(Point::pos)>::type::ElementType u) const override
	{
		return spline(u);
	}
};

void AWalkthroughCameraPawn::TSplineDeleter::operator ()(ISpline *spline) const
{
	delete spline;
}

// Sets default values
AWalkthroughCameraPawn::AWalkthroughCameraPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	cameraComponent = FObjectInitializer::Get().CreateDefaultSubobject<UCameraComponent>(this, TEXT("WalkthroughCamera"));
	cameraComponent->RelativeLocation = FVector::ZeroVector;
	cameraComponent->bUsePawnControlRotation = false;
}

AWalkthroughCameraPawn::~AWalkthroughCameraPawn() = default;

// Called every frame
void AWalkthroughCameraPawn::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

	if (time <= 1)
	{
		assert(spline);

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

		const auto point = spline->operator ()(u);
		const auto &rot = std::get<0>(point.attribs);
		cameraComponent->SetWorldLocationAndRotation({ point.pos.x, point.pos.y, point.pos.z }, { rot.x, rot.y, rot.z });

		if (time > 1)
			OnFinish.Broadcast();
	}
}

void AWalkthroughCameraPawn::Run(float speed, float transient, bool uniformSpeed)
{
	std::list<std::pair<Point, int>> points;
	for (TActorIterator<AInterpolationPointActor> actor(GetWorld()); actor; ++actor)
	{
		const auto pos = actor->GetActorLocation();
		const auto rot = actor->GetActorRotation();
		points.emplace_back(std::piecewise_construct, std::make_tuple(decltype(Point::pos)(pos.X, pos.Y, pos.Z), std::tuple_element<0, decltype(Point::attribs)>::type(rot.Pitch, rot.Roll, rot.Yaw)), std::make_tuple(actor->id));
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
			const float offset = Math::VectorMath::distance(prevPoint++->first.pos, point.first.pos);
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
		const Point &operator *() const { return TBase::operator *().first; }
		const Point *operator ->() const { return &TBase::operator ->()->first; }
		CPointIterator operator ++() { return TBase::operator ++(); }
		CPointIterator operator --() { return TBase::operator --(); }
		CPointIterator operator ++(int) { return TBase::operator ++(0); }
		CPointIterator operator --(int) { return TBase::operator --(0); }
	};

	CPointIterator begin(points.cbegin()), end(points.cend());
	spline = uniformSpeed
#if 0
		? static_cast<decltype(spline)>(std::make_unique<CSplineProxy<TBesselOverhauser>>	(std::move(begin), std::move(end)))
		: static_cast<decltype(spline)>(std::make_unique<CSplineProxy<TCatmullRom>>			(std::move(begin), std::move(end)));
#else
		? static_cast<decltype(spline)>(std::unique_ptr<CSplineProxy<TBesselOverhauser>,	TSplineDeleter>(new CSplineProxy<TBesselOverhauser>	(std::move(begin), std::move(end))))
		: static_cast<decltype(spline)>(std::unique_ptr<CSplineProxy<TCatmullRom>,			TSplineDeleter>(new CSplineProxy<TCatmullRom>		(std::move(begin), std::move(end))));
#endif
}

void AWalkthroughCameraPawn::Stop()
{
	time = std::numeric_limits<float>::infinity();
	spline.reset();
}