#include "FlightSimulator.h"

static constexpr double DEG2RAD = M_PI / 180.0;

FlightSimulator::FlightSimulator() {
}

template <FlightSimulator::SimOpt SIM>
void FlightSimulator::simulate_launch(const Body& body, const RocketConfig& rocket,
                                      float gtClimbAlt, float gtTurnSpread, float gtFinalPitch) {
    vec2f pos {0, body.radius_km};
    vec2f vel {static_cast<float>(2.0f * M_PI * body.radius_km * 1000.0f / body.rotPeriod_s), 0};
    vec2f dir {0, 1};
    launchSuccess = LaunchSuccess {};

    std::vector<StageKinematics> kinematics;
    rocket.calcStageKinematics(kinematics);

    const int nStage = kinematics.size();
    int stage = 0;
    StageKinematics currentStage = kinematics[0];
    float elapsed = 0;
    float stageTime = 0;
    float mass = currentStage.m0;
    float flowRate = (currentStage.m0 - currentStage.mf) / currentStage.burnTime;
    const float GM = body.GM();

    float totalBurnTime = 0;
    for (auto& k : kinematics) { totalBurnTime += k.burnTime; }
    int maxIter = static_cast<int>((totalBurnTime / dt) + 200);

    if constexpr (SIM == SimOpt::RECORD) {
        flight_data.clear();
        flight_data.reserve(maxIter);
    }

    bool circularizationChecked = false;

    for (int i = 0; i < maxIter; ++i) {
        if (stageTime > currentStage.burnTime) {
            // Staging event
            if (stage == nStage - 1) {
                break;
            }
            stage++;
            currentStage = kinematics[stage];
            stageTime = 0;
            mass = currentStage.m0;
            flowRate = (currentStage.m0 - currentStage.mf) / currentStage.burnTime;
        }

        float R = std::sqrt(pos.x*pos.x + pos.y*pos.y);
        float altitude = R - body.radius_km;
        float pressure = body.getPressureAtAltitude_km(altitude);

        float g_mag = body.surfaceGravity * std::pow(body.radius_km / R, 2);
        vec2f grav = {-g_mag * pos.x / R, -g_mag * pos.y / R};

        // Calculate current, pressure dependent thrust
        float isp = currentStage.engine->enginePerf.getISP(pressure);
        float ispVac = currentStage.engine->enginePerf.vacuumISP;
        float thrust = (isp / ispVac) * currentStage.engine->MaxThrustkN * currentStage.nEngines;

        // Calculate rocket orientation w.r.t planet center
        vec2f up {pos.x / R, pos.y / R};
        vec2f east {up.y, -up.x};

        // Altitude-based turn schedule
        float progress = std::clamp((altitude - gtClimbAlt) / gtClimbAlt, 0.0f, 1.0f);
        float shaped   = std::pow(progress, gtTurnSpread);
        float schedulePitch = shaped * gtFinalPitch * DEG2RAD;

        // TWR-balanced pitch
        float twrPitch = thrust > 0 ? std::acos(std::clamp(mass * g_mag / thrust, 0.0f, 1.0f)) : 0.0f;

        // Blend: schedule dominates early in the turn (climb phase),
        // TWR dominates later. Smoothly transitions with progress.
        float pitch_angle = schedulePitch * (1.0f - progress) + twrPitch * progress;

        dir.x = cos(pitch_angle) * up.x + sin(pitch_angle) * east.x;
        dir.y = cos(pitch_angle) * up.y + sin(pitch_angle) * east.y;

        mass -= flowRate * dt;
        float A = currentStage.area_m2;
        float air_density = (body.seaLevel_atm > 0) ? pressure * body.sea_level_density_kgpm3 / body.seaLevel_atm : 0.0;
        float speed = std::sqrtf(vel.x * vel.x + vel.y * vel.y);
        float drag_mag = 0.5 * air_density * speed * speed * A * Cd;
        vec2f drag_accel {0, 0};
        if (speed > 1e-6) {
            float ax = (-drag_mag / (mass * 1000.0) * vel.x / speed);
            float ay = (-drag_mag / (mass * 1000.0) * vel.y / speed);
            drag_accel = {ax, ay};
        }

        float accel_x = thrust * dir.x / mass + drag_accel.x + grav.x;
        float accel_y = thrust * dir.y / mass + drag_accel.y + grav.y;
        vel.x += accel_x * dt;
        vel.y += accel_y * dt;
        pos.x += vel.x * dt / 1000.0;
        pos.y += vel.y * dt / 1000.0;

        const float APO = getOrbitExtent<Apoapsis>(pos, vel, R, GM) - body.radius_km;

        if (!circularizationChecked && APO > body.atmHeight_km + 10.0) {
            // Calculate whether it is possible to circularize the orbit
            circularizationChecked = true;
            float PER = getOrbitExtent<Periapsis>(pos, vel, R, GM) - body.radius_km;
            float a = (APO + PER) / 2.0 + body.radius_km;
            float va = sqrt(GM * (2.0 / (APO + body.radius_km) - 1.0 / a));
            float vc = sqrt(GM / (APO + body.radius_km));
            float circ_dV = (vc - va) * 1000.0f;
            float avail_dV = rocket.remainingDeltaV(kinematics, elapsed);

            launchSuccess.apoapsis_safe_height = APO > body.atmHeight_km + 10.0;
            launchSuccess.circularization_dv = circ_dV;
            launchSuccess.availableDeltaV = avail_dV;
        }

        if constexpr (SIM == SimOpt::RECORD) {
            flight_data.t.push_back(elapsed);
            flight_data.altitude_km.push_back(altitude);
            flight_data.velocity_ms.push_back(speed);
            flight_data.vx_ms.push_back(vel.x);
            flight_data.vy_ms.push_back(vel.y);
            flight_data.posx_km.push_back(pos.x);
            flight_data.posy_km.push_back(pos.y);
            flight_data.thrust_kN.push_back(thrust);
            flight_data.mass_t.push_back(mass);
            flight_data.pressure_atm.push_back(pressure);
            flight_data.dir_angle_deg.push_back(pitch_angle * 180.0 / M_PI);
            flight_data.apoapsis_km.push_back(APO);
            flight_data.stage.push_back(stage);
            flight_data.area_m2.push_back(A);
            flight_data.drag_N.push_back(drag_mag);
        }

        stageTime += dt;
        elapsed += dt;
    }
}

template void FlightSimulator::simulate_launch<FlightSimulator::SimOpt::RECORD>(const Body&, const RocketConfig&, float, float, float);
template void FlightSimulator::simulate_launch<FlightSimulator::SimOpt::FAST>(const Body&, const RocketConfig&, float, float, float);
