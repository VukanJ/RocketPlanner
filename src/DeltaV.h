#ifndef DELTAV_H
#define DELTAV_H


#include "kspConstants.h"


float naiveTakeoffLandingCost(const Body* body, float targetAltitudeKm);

float escapeBurnCost(const Body* body, float targetAltitudeKm);

Orbit getHohmannOrbit(const Body* origin, const Body* target, float startAltitude);

float hohmannTransferCost_Planet2Planet(const Body* origin, const Body* target);
float hohmannTransferCost_Planet2Moon(const Body* origin, const Body* target, float startAltitude);

float circularizeHyperbolicCost(const Body* origin, const Body* target, float startAltitude, float rmin, bool prograde = true);

float directionToAnomaly(const Eigen::Vector3f& dir, const Orbit& orbit);

float inclinationCorrectionCost(const Orbit& origin, const Orbit& target);



#endif // DELTAV_H
