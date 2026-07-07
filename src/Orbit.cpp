#include "Orbit.h"
#include "kspConstants.h"

#include <cmath>

float Orbit::v_apoapsis(float unit) const {
    return unit * std::sqrt(parent->GM() * (2.0f / AP - 1.0f / a_semi));
}

float Orbit::v_periapsis(float unit) const {
    return unit * std::sqrt(parent->GM() * (2.0f / PE - 1.0f / a_semi));
}
