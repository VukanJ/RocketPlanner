#ifndef DELTAV_H
#define DELTAV_H


#include "kspConstants.h"

struct DeltaVRange {
    DeltaVRange() = default;
    DeltaVRange(float dv) { add(dv); }
    DeltaVRange& operator=(float dv) {
        min = dv;
        max = dv;
        return *this;
    }
    void add(float dv) {
        if (dv > max) { max = dv; }
        if (dv < min) { min = dv; }
    }
    bool single() const {
        return min == max;
    }
    bool has_value() const {
        return min < INFINITY && max > -INFINITY;
    }

    float min = INFINITY;
    float max = -INFINITY;
};


float naiveTakeoffLandingCost(const Body* body, float targetAltitudeKm);

float escapeBurnCost(const Body* body, float targetAltitudeKm);

Orbit getHohmannOrbit(const Body* origin, const Body* target, float startAltitude);

DeltaVRange hohmannTransferCost_Planet2Planet(const Body* origin, const Body* target);
DeltaVRange hohmannTransferCost_Planet2Moon(const Body* origin, const Body* target, float startAltitude);

float circularizeHyperbolicCost(const Body* origin, const Body* target, float startAltitude, float rmin, bool prograde = true);

float directionToAnomaly(const Eigen::Vector3f& dir, const Orbit& orbit);

float inclinationCorrectionCost(const Orbit& origin, const Orbit& target);



#endif // DELTAV_H
