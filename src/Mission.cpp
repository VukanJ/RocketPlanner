#include "Mission.h"
#include "WindowMission.h"
#include "helper.h"

void Mission::updateMissionSequence(std::vector<MissionPhase>& msequence, const Mission& mission, bool advanced) {
    msequence.clear();

    auto fromTo = [&](const Body* from, const Body* to) {
        float startOrbit = from->atmHeight_km > 0 ? from->atmHeight_km + 10.0 : 30;
        float destOrbit  = to->atmHeight_km > 0 ? to->atmHeight_km + 10.0 : 30;
        float reentryPE  = destOrbit;

        if (to == from->orbit.parent) {
            reentryPE = 30;  // guarantee atmospheric capture
        }

        msequence.push_back(MissionPhase::takeoff(from, startOrbit));

        if (from == to) {
            if (to->atmHeight_km > 0) {
                msequence.push_back(MissionPhase { MissionPhase::Type::LANDING_PARACHUTES, from, nullptr, startOrbit });
            }
            else {
                msequence.push_back(MissionPhase { MissionPhase::Type::LANDING, from, nullptr, destOrbit });
            }
        }
        else {
            if (isPlanetToPlanet(from, to)) {
                // Planet to planet — both orbit the same parent body
                if (advanced) {
                    msequence.push_back(MissionPhase { MissionPhase::Type::ORBITAL_INSERTION, from, to, startOrbit, destOrbit });
                }
                else {
                    msequence.push_back(MissionPhase { MissionPhase::Type::ESCAPE, from, to, startOrbit, destOrbit });
                    if (to->orbit.inclination != from->orbit.inclination) {
                        msequence.push_back(MissionPhase { MissionPhase::Type::INCLINATION_CORRECTION, from, to, startOrbit });
                    }
                    msequence.push_back(MissionPhase::hohmann(from, to, startOrbit, destOrbit));
                }
            }
            if (to == from->orbit.parent) {
                // Returning from a natural satellite to its parent body
                msequence.push_back(MissionPhase { MissionPhase::Type::ESCAPE, from, to, startOrbit, reentryPE });
            }
            else if (isPlanetToMoon(from, to)) {
                // Parent to moon — inclination correction w.r.t. parent's equator
                if (advanced) {
                    msequence.push_back(MissionPhase { MissionPhase::Type::ORBITAL_INSERTION, from, to, startOrbit, destOrbit });
                }
                else {
                    if (to->orbit.inclination != 0) { // origin orbit is in parents ecliptic
                        msequence.push_back(MissionPhase { MissionPhase::Type::INCLINATION_CORRECTION, from, to, startOrbit });
                    }
                    msequence.push_back(MissionPhase::hohmann(from, to, startOrbit, destOrbit));
                }
            }
        }
        if (to->atmHeight_km > 0) {
            msequence.push_back(MissionPhase { MissionPhase::Type::ATMOSPHERIC_BREAKING, to, nullptr, reentryPE });
            msequence.push_back(MissionPhase { MissionPhase::Type::LANDING_PARACHUTES, to, nullptr, destOrbit });
        }
        else {
            msequence.push_back(MissionPhase::circularize_hyperbolic(from, to, startOrbit, destOrbit));
            msequence.push_back(MissionPhase { MissionPhase::Type::LANDING, to, nullptr, destOrbit });
        }
    };
    fromTo(mission.originBody, mission.destinationBody);
    if (!mission.oneWayTrip) {
        fromTo(mission.destinationBody, mission.originBody);
    }

    for (auto& step : msequence) {
        step.updateDeltaV();
    }

    // Initialize porkchop plots
    for (auto& step : msequence) {
        if (step.type == MissionPhase::Type::ORBITAL_INSERTION) {
            step.porkchopPlot = std::make_unique<PorkchopPlot>(&step);
        }
    }
}

std::string PhaseToString(MissionPhase::Type type) {
    switch (type) {
        case MissionPhase::Type::TAKEOFF:                return "Takeoff";
        case MissionPhase::Type::CIRCULARIZE_HYPERBOLIC: return "Circularize Hyperbolic";
        case MissionPhase::Type::ESCAPE:                 return "Escape";
        case MissionPhase::Type::HOHMANN_TRANSFER:       return "Hohmann Transfer";
        case MissionPhase::Type::ATMOSPHERIC_BREAKING:   return "Atmospheric Breaking";
        case MissionPhase::Type::LANDING_PARACHUTES:     return "Landing with Parachutes";
        case MissionPhase::Type::INCLINATION_CORRECTION: return "Inclination Correction";
        case MissionPhase::Type::ORBITAL_INSERTION:      return "Orbital Insertion";
        case MissionPhase::Type::LANDING:                return "Landing";
        case MissionPhase::Type::MINING:                 return "Mining";
        case MissionPhase::Type::ORBITAL_REFUELING:      return "Orbital Refueling";
        default:                                         return "Unknown Phase";
    }
}

void MissionPhase::updateDeltaV() {
    switch (type) {
        case MissionPhase::Type::LANDING: 
            dv_range = naiveTakeoffLandingCost(refBody, alt1); 
            break;
        case MissionPhase::Type::TAKEOFF: 
            dv_range = naiveTakeoffLandingCost(refBody, alt1); 
            break;
        case MissionPhase::Type::ESCAPE:
            if (isPlanetToPlanet(refBody, refBody2)) {
                // Planet to planet, or Moon to Moon
                // No direct maneuver, since inclination correction might be needed.
                dv_range = escapeBurnCost(refBody, alt1);
            }
            else {
                // Combine Escape and Hohmann return into a single maneuver
                // Cost should be the same as for circularizing hyperbolic orbit 
                // after a Hohmann transfer, just in reverse. (Time-reversal symmetry)
                dv_range = circularizeHyperbolicCost(refBody->orbit.parent, refBody, alt2, alt1, true);
            }
            break;
        case MissionPhase::Type::CIRCULARIZE_HYPERBOLIC:
            if (refBody2 == refBody->orbit.parent) {
                dv_range = circularizeHyperbolicCost(refBody2, refBody, alt2, alt1, true);
            } else if (refBody2 && isPlanetToPlanet(refBody, refBody2)) {
                // Planet -> planet
                dv_range = circularizeHyperbolicCost(refBody, refBody2, alt1, alt2, true);
            } else {
                dv_range = circularizeHyperbolicCost(refBody, refBody2, alt1, alt2, true);
            }
            break;
        case MissionPhase::Type::HOHMANN_TRANSFER:
            dv_range = hohmannTransferCost(refBody, refBody2, alt1);
            break;
        case MissionPhase::Type::ORBITAL_INSERTION:
            dv_range = orbitalInsertion(refBody, refBody2, alt1);
            break;
        case MissionPhase::Type::INCLINATION_CORRECTION:
            if (isPlanetToMoon(refBody, refBody2)) {
                // Planet → moon: ship's parking orbit vs moon's orbit around planet
                Orbit shipOrbit = Orbit::circular(refBody, alt1 + refBody->radius_km);
                dv_range = inclinationCorrectionCost(shipOrbit, refBody2->orbit);
            } else {
                // Planet → planet: orbits around shared parent
                dv_range = inclinationCorrectionCost(refBody->orbit, refBody2->orbit);
            }
            break;
        default: dv_range = 0;
    }
}
