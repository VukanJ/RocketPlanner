#include "FlightSimulator.h"

FlightData<float> simulate_flight(Body body, const RocketConfig& rocket, LaunchSuccess& launchSuccess) {
    struct vec { float x = 0; float y = 0; };
    vec pos {0, (float)body.radius_km};
    vec vel {(float)(2.0f * M_PI * body.radius_km * 1000.0f / body.rotPeriod_s), 0};
    vec dir {0, 1};

    FlightData<float> data;

    std::vector<StageKinematics> kinematics;
    rocket.calcStageKinematics(kinematics);

    auto getApoapsis = [&pos, &vel, &body]() -> float {
        double vx = vel.x / 1000.0;
        double vy = vel.y / 1000.0;
        double v2 = vx*vx + vy*vy;
        double r  = std::sqrt(pos.x*pos.x + pos.y*pos.y);
        double eps = 0.5*v2 - body.GM() / r;
        if (eps >= 0) { return INFINITY; }
        double a   = -body.GM() / (2*eps);
        double h   = pos.x * vy - pos.y * vx;
        double e   = std::sqrt(1 + 2*eps*h*h / (body.GM()*body.GM()));
        return a * (1 + e) - body.radius_km;
    };

    auto getPeriapsis = [&pos, &vel, &body]() -> float {
        double vx = vel.x / 1000.0;
        double vy = vel.y / 1000.0;
        double v2 = vx*vx + vy*vy;
        double r  = std::sqrt(pos.x*pos.x + pos.y*pos.y);
        double eps = 0.5*v2 - body.GM() / r;
        if (eps >= 0) { return INFINITY; }
        double a   = -body.GM() / (2*eps);
        double h   = pos.x * vy - pos.y * vx;
        double e   = std::sqrt(1 + 2*eps*h*h / (body.GM()*body.GM()));
        return a * (1 - e) - body.radius_km;
    };

    float dt = 0.1;

    const int nStage = kinematics.size();
    int stage = 0;
    StageKinematics currentStage = kinematics[0];
    float elapsed = 0;
    float stageTime = 0;
    float mass = currentStage.m0;
    float flowRate = (currentStage.m0 - currentStage.mf) / currentStage.burnTime;

    float totalBurnTime = 0;
    for (auto& k : kinematics) totalBurnTime += k.burnTime;
    int maxIter = (int)(totalBurnTime / dt) + 200;

    data.reserve(maxIter);

    bool circularizationChecked = false;

    for (int i = 0; i < maxIter; ++i) {
        if (stageTime > currentStage.burnTime) {
            if (stage == nStage - 1) {
                break;
            }
            stage++;
            currentStage = kinematics[stage];
            stageTime = 0;
            mass = currentStage.m0;
            flowRate = (currentStage.m0 - currentStage.mf) / currentStage.burnTime;
        }

        float altitude = std::sqrt(pos.x*pos.x + pos.y*pos.y) - body.radius_km;
        float pressure = body.getPressureAtAltitude_km(altitude);

        float r = altitude + body.radius_km;
        float g_mag = body.surfaceGravity * std::pow(body.radius_km / r, 2);
        vec grav = {-g_mag * pos.x / r, -g_mag * pos.y / r};

        {
            float X = std::sqrt(pos.x*pos.x + pos.y*pos.y);
            vec up  {pos.x / X, pos.y / X};
            vec east{up.y, -up.x};
            float angle_rad;
            constexpr float startATM = 1.0f;
            if (pressure > startATM) {
                angle_rad = 0.0f;
            }
            else if (pressure <= 0.0f) {
                angle_rad = 85.0f * M_PI / 180.0f;
            }
            else {
                float t = pressure / startATM;
                angle_rad = (1.0f - t) * 85.0f * M_PI / 180.0f;
            }
            dir.x = cos(angle_rad) * up.x + sin(angle_rad) * east.x;
            dir.y = cos(angle_rad) * up.y + sin(angle_rad) * east.y;
        }

        float isp = currentStage.engine->enginePerf.getISP(pressure);
        float ispVac = currentStage.engine->enginePerf.vacuumISP;
        float thrust = (isp / ispVac) * currentStage.engine->MaxThrustkN * currentStage.nEngines;

        double A = currentStage.area_m2;
        constexpr double Cd = 0.2;
        double rho = (body.seaLevel_atm > 0) ? (double)pressure * body.sea_level_density_kgpm3 / body.seaLevel_atm : 0.0;
        double speed_d = std::sqrt((double)vel.x * vel.x + (double)vel.y * vel.y);
        double drag_mag = 0.5 * rho * speed_d * speed_d * A * Cd;
        float drag_N = (float)drag_mag;
        vec drag_accel{0, 0};
        if (speed_d > 1e-6) {
            float ax = (float)(-drag_mag / (mass * 1000.0) * vel.x / speed_d);
            float ay = (float)(-drag_mag / (mass * 1000.0) * vel.y / speed_d);
            drag_accel = {ax, ay};
        }

        float accel_x = thrust * dir.x / mass + drag_accel.x + grav.x;
        float accel_y = thrust * dir.y / mass + drag_accel.y + grav.y;
        vel.x += accel_x * dt;
        vel.y += accel_y * dt;
        pos.x += vel.x * dt / 1000.0;
        pos.y += vel.y * dt / 1000.0;

        mass -= flowRate * dt;

        float apo = getApoapsis();
        float speed = std::sqrt(vel.x*vel.x + vel.y*vel.y);

        if (!circularizationChecked && apo > body.atmHeight_km + 10.0) {
            circularizationChecked = true;
            float periapsis = getPeriapsis();
            float a = (apo + periapsis) / 2.0 + body.radius_km;
            float va = sqrt(body.GM() * (2.0 / (apo + body.radius_km) - 1.0 / a));
            float vc = sqrt(body.GM() / (apo + body.radius_km));
            float circ_dV = (vc - va) * 1000.0f;
            float avail_dV = rocket.remainingDeltaV(kinematics, elapsed);

            launchSuccess.apoapsis_safe_height = apo > body.atmHeight_km + 10.0;
            launchSuccess.circularization_dv = circ_dV;
            launchSuccess.availableDeltaV = avail_dV;
        }

        double dir_angle_deg = std::atan2(dir.x, dir.y) * 180.0 / M_PI;
        data.t.push_back(elapsed);
        data.altitude_km.push_back(altitude);
        data.velocity_ms.push_back(speed);
        data.vx_ms.push_back(vel.x);
        data.vy_ms.push_back(vel.y);
        data.posx_km.push_back(pos.x);
        data.posy_km.push_back(pos.y);
        data.thrust_kN.push_back(thrust);
        data.mass_t.push_back(mass);
        data.pressure_atm.push_back(pressure);
        data.dir_angle_deg.push_back(dir_angle_deg);
        data.apoapsis_km.push_back(apo);
        data.stage.push_back(stage);
        data.area_m2.push_back(A);
        data.drag_N.push_back(drag_N);

        stageTime += dt;
        elapsed += dt;
    }
    return data;
}

