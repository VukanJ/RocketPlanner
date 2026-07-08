#include "Orbit.h"
#include "kspConstants.h"
#include "utils.h"
#include "eigen3/Eigen/Geometry"

#include <cmath>

float Orbit::v_apoapsis(float unit) const {
    return unit * std::sqrt(parent->GM() * (2.0f / AP - 1.0f / a_semi));
}

float Orbit::v_periapsis(float unit) const {
    return unit * std::sqrt(parent->GM() * (2.0f / PE - 1.0f / a_semi));
}

Eigen::Vector3f Orbit::normal() const {
    // Calculate normal vector of orbital plane
    Eigen::Vector3f up(0, 1, 0);
    Eigen::Vector3f solarprimevector(1, 0, 0);

    auto ANdir = Eigen::AngleAxisf(deg2rad(-LAN), up) * solarprimevector;
    auto N = Eigen::AngleAxisf(deg2rad(-inclination), ANdir) * up;
    if (N.y() < 0) { N = -N; }

    return N;
}
