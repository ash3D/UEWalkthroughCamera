#include "Engine.h"

#pragma warning(default : 4396 4503 4624 4800)

#define DISABLE_MATRIX_SWIZZLES
#include "vector math.h"
#include "splines.h"
#include "hermite.h"
#include <list>
#include <iterator>
#include <iostream>
#include <numeric>
#include <utility>
#include <type_traits>
#include <tuple>
#include <cassert>
#include <cstdint>
#include <cmath>