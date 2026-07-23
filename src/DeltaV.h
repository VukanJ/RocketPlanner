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

float directionToAnomaly(const Eigen::Vector3f& dir, const Orbit& orbit);
Orbit getHohmannOrbit(const Body* center, float R1, float R2);

float naiveTakeoffLandingCost(const Body* body, float targetAltitudeKm);
float escapeBurnCost(const Body* body, float targetAltitudeKm);
DeltaVRange hohmannTransferCost(const Body* origin, const Body* target, float startAltitude = 0);
DeltaVRange circularizeHyperbolicCost(const Body* origin, const Body* target, float startAltitude, float rmin, bool prograde = true);
DeltaVRange inclinationCorrectionCost(const Orbit& origin, const Orbit& target);
DeltaVRange orbitalInsertion(const Body* origin, const Body* target, float startAltitude);

float orbitalInsertion(const Body* origin, const vec3f& v_origin, const vec3f& v_oo, float startAltitude);
float circularizeHyperbolicCost(const Body* target, const vec3f& v_target, const vec3f& v_oo, float targetAltitudeKm);

#endif // DELTAV_H
