#ifndef KSPCONSTANTS_H
#define KSPCONSTANTS_H

#include <cmath>
#include "Orbit.h"

namespace Constants {
    // Densities in tons/unit liter
    constexpr inline double LiquidFuelDensity     = 0.005;
    constexpr inline double OxidizerDensity       = 0.005;
    constexpr inline double MonoPropellantDensity = 0.004;
    constexpr inline double SolidFuelDensity      = 0.0075;
    constexpr inline double XenonGasDensity       = 0.0001;

    constexpr inline double mk0_area_m2 = M_PI * 0.625 * 0.625;
    constexpr inline double mk1_area_m2 = M_PI * 1.25 * 1.25;
    constexpr inline double mk2_area_m2 = M_PI * 1.9 * 1.9;
    constexpr inline double mk3_area_m2 = M_PI * 2.5 * 2.5;

    constexpr inline double atmPascal = 101325.0;
    constexpr inline double RGas_J_kgK = 287.053; // Specific gas constant 
};

struct Body {
    const char* name;
    const float radius_km;
    const float R_SOI_km; // sphere of influence in km
    const double surfaceGravity;
    const float seaLevel_atm; // in atm
    const float atmHeight_km;
    const float atm_falloff_km; // Approximate atm scale parameter
    const float sea_level_density_kgpm3;
    const float rotPeriod_s;
    const double GM_km3s2;
    const Orbit orbit;

    inline bool hasAtmosphere() const {
        return seaLevel_atm > 0;
    }

    float getPressureAtAltitude_km(float altitude) const {
        if (altitude >= atmHeight_km || seaLevel_atm <= 0.0f) { 
            return 0.0f; 
        }
        return seaLevel_atm * std::expf(-altitude / atm_falloff_km);
    }
};


namespace KspSystem {
    
    constexpr inline Body Kerbol  {
        .name                    = "Kerbol",
        .radius_km               = 261600.0,
        .R_SOI_km                = INFINITY,
        .surfaceGravity          = 17.1307128274441,
        .seaLevel_atm            = 0.157907722674562,
        .atmHeight_km            = 600.0,
        .atm_falloff_km          = 0.0,
        .sea_level_density_kgpm3 = 0.00072492861572823,
        .rotPeriod_s             = 432000.0,
        .GM_km3s2                = 1172332794.83249,
        .orbit                   = {} };

    constexpr inline Body Moho  {
        .name                    = "Moho",
        .radius_km               = 250.0,
        .R_SOI_km                = 9646.66302332811,
        .surfaceGravity          = 2.69775005847215,
        .seaLevel_atm            = 0.0,
        .atmHeight_km            = 0.0,
        .atm_falloff_km          = 0.0,
        .sea_level_density_kgpm3 = 0.0,
        .rotPeriod_s             = 1210000.0,
        .GM_km3s2                = 168.609378654509,
        .orbit={.parent=&KspSystem::Kerbol,
                .LAN                 = 70.0,
                .LDN                 = 250.0,
                .argumentOfPeriapsis = 15.0,
                .meanAnomaly         = 3.14000010490417,
                .AP                  = 6315765.98048537,
                .PE                  = 4210510.62751463,
                .a_semi              = 5263138.304,
                .eccentricity        = 0.200000002980232,
                .inclination         = 7.0,
                .epoch               = 0.0
        } };

    constexpr inline Body Eve  {
        .name                    = "Eve",
        .radius_km               = 700.0,
        .R_SOI_km                = 85109.3647382441,
        .surfaceGravity          = 16.6770004677773,
        .seaLevel_atm            = 5.0,
        .atmHeight_km            = 90.0,
        .atm_falloff_km          = 8.67283741733484,
        .sea_level_density_kgpm3 = 6.23837138885624,
        .rotPeriod_s             = 80500.0,
        .GM_km3s2                = 8171.73022921087,
        .orbit={.parent=&KspSystem::Kerbol,
                .LAN                 = 15.0,
                .LDN                 = 195.0,
                .argumentOfPeriapsis = 0.0,
                .meanAnomaly         = 3.14000010490417,
                .AP                  = 9931011.38724222,
                .PE                  = 9734357.70075778,
                .a_semi              = 9832684.544,
                .eccentricity        = 0.00999999977648258,
                .inclination         = 2.09999990463257,
                .epoch               = 0.0
        } };

    constexpr inline Body Gilly  {
        .name                    = "Gilly",
        .radius_km               = 13.0,
        .R_SOI_km                = 126.123271704568,
        .surfaceGravity          = 0.0490499989036471,
        .seaLevel_atm            = 0.0,
        .atmHeight_km            = 0.0,
        .atm_falloff_km          = 0.0,
        .sea_level_density_kgpm3 = 0.0,
        .rotPeriod_s             = 28255.0,
        .GM_km3s2                = 0.00828944981471635,
        .orbit={.parent=&KspSystem::Eve,
                .LAN                 = 80.0,
                .LDN                 = 260.0,
                .argumentOfPeriapsis = 10.0,
                .meanAnomaly         = 0.899999976158142,
                .AP                  = 48825.0003755093,
                .PE                  = 14174.9996244907,
                .a_semi              = 31500.0,
                .eccentricity        = 0.550000011920929,
                .inclination         = 12.0,
                .epoch               = 0.0
        } };

    constexpr inline Body Kerbin  {
        .name                    = "Kerbin",
        .radius_km               = 600.0,
        .R_SOI_km                = 84159.2864796305,
        .surfaceGravity          = 9.81000000000002,
        .seaLevel_atm            = 0.99999996988149,
        .atmHeight_km            = 70.0,
        .atm_falloff_km          = 5.92346848549138,
        .sea_level_density_kgpm3 = 1.22497705725583,
        .rotPeriod_s             = 21549.4251830898,
        .GM_km3s2                = 3531.6,
        .orbit={.parent=&KspSystem::Kerbol,
                .LAN                 = 0.0,
                .LDN                 = 180.0,
                .argumentOfPeriapsis = 0.0,
                .meanAnomaly         = 3.14000010490417,
                .AP                  = 13599840.256,
                .PE                  = 13599840.256,
                .a_semi              = 13599840.256,
                .eccentricity        = 0.0,
                .inclination         = 0.0,
                .epoch               = 0.0
        } };

    constexpr inline Body Mun  {
        .name                    = "Mun",
        .radius_km               = 200.0,
        .R_SOI_km                = 2429.55911656475,
        .surfaceGravity          = 1.62845993801951,
        .seaLevel_atm            = 0.0,
        .atmHeight_km            = 0.0,
        .atm_falloff_km          = 0.0,
        .sea_level_density_kgpm3 = 0.0,
        .rotPeriod_s             = 138984.376574476,
        .GM_km3s2                = 65.1383975207807,
        .orbit={.parent=&KspSystem::Kerbin,
                .LAN                 = 0.0,
                .LDN                 = 180.0,
                .argumentOfPeriapsis = 0.0,
                .meanAnomaly         = 1.70000004768372,
                .AP                  = 12000.0,
                .PE                  = 12000.0,
                .a_semi              = 12000.0,
                .eccentricity        = 0.0,
                .inclination         = 0.0,
                .epoch               = 0.0
        } };

    constexpr inline Body Minmus  {
        .name                    = "Minmus",
        .radius_km               = 60.0,
        .R_SOI_km                = 2247.4283879023,
        .surfaceGravity          = 0.49050000730902,
        .seaLevel_atm            = 0.0,
        .atmHeight_km            = 0.0,
        .atm_falloff_km          = 0.0,
        .sea_level_density_kgpm3 = 0.0,
        .rotPeriod_s             = 40400.0,
        .GM_km3s2                = 1.76580002631247,
        .orbit={.parent=&KspSystem::Kerbin,
                .LAN                 = 78.0,
                .LDN                 = 258.0,
                .argumentOfPeriapsis = 38.0,
                .meanAnomaly         = 0.899999976158142,
                .AP                  = 47000.0,
                .PE                  = 47000.0,
                .a_semi              = 47000.0,
                .eccentricity        = 0.0,
                .inclination         = 6.0,
                .epoch               = 0.0
        } };

    constexpr inline Body Duna  {
        .name                    = "Duna",
        .radius_km               = 320.0,
        .R_SOI_km                = 47921.949369738,
        .surfaceGravity          = 2.94300011694432,
        .seaLevel_atm            = 0.0666666677961107,
        .atmHeight_km            = 50.0,
        .atm_falloff_km          = 6.94209577551112,
        .sea_level_density_kgpm3 = 0.149935108881759,
        .rotPeriod_s             = 65517.859375,
        .GM_km3s2                = 301.363211975098,
        .orbit={.parent=&KspSystem::Kerbol,
                .LAN                 = 135.5,
                .LDN                 = 315.5,
                .argumentOfPeriapsis = 0.0,
                .meanAnomaly         = 3.14000010490417,
                .AP                  = 21783189.162698,
                .PE                  = 19669121.365302,
                .a_semi              = 20726155.264,
                .eccentricity        = 0.0509999990463257,
                .inclination         = 0.0599999986588955,
                .epoch               = 0.0
        } };

    constexpr inline Body Ike  {
        .name                    = "Ike",
        .radius_km               = 130.0,
        .R_SOI_km                = 1049.59893931162,
        .surfaceGravity          = 1.09872003391385,
        .seaLevel_atm            = 0.0,
        .atmHeight_km            = 0.0,
        .atm_falloff_km          = 0.0,
        .sea_level_density_kgpm3 = 0.0,
        .rotPeriod_s             = 65517.8621348081,
        .GM_km3s2                = 18.568368573144,
        .orbit={.parent=&KspSystem::Duna,
                .LAN                 = 0.0,
                .LDN                 = 180.0,
                .argumentOfPeriapsis = 0.0,
                .meanAnomaly         = 1.70000004768372,
                .AP                  = 3295.99999785423,
                .PE                  = 3104.00000214577,
                .a_semi              = 3200.0,
                .eccentricity        = 0.0299999993294477,
                .inclination         = 0.200000002980232,
                .epoch               = 0.0
        } };

    constexpr inline Body Dres  {
        .name                    = "Dres",
        .radius_km               = 138.0,
        .R_SOI_km                = 32832.8395767762,
        .surfaceGravity          = 1.12815,
        .seaLevel_atm            = 0.0,
        .atmHeight_km            = 0.0,
        .atm_falloff_km          = 0.0,
        .sea_level_density_kgpm3 = 0.0,
        .rotPeriod_s             = 34800.0,
        .GM_km3s2                = 21.4844886,
        .orbit={.parent=&KspSystem::Kerbol,
                .LAN                 = 280.0,
                .LDN                 = 460.0,
                .argumentOfPeriapsis = 90.0,
                .meanAnomaly         = 3.14000010490417,
                .AP                  = 46761053.692435,
                .PE                  = 34917642.713565,
                .a_semi              = 40839348.203,
                .eccentricity        = 0.145,
                .inclination         = 5.0,
                .epoch               = 0.0
        } };

    constexpr inline Body Jool  {
        .name                    = "Jool",
        .radius_km               = 6000.0,
        .R_SOI_km                = 2455985.18542347,
        .surfaceGravity          = 7.84800011694431,
        .seaLevel_atm            = 15.0,
        .atmHeight_km            = 200.0,
        .atm_falloff_km          = 0.0,
        .sea_level_density_kgpm3 = 6.70262205528434,
        .rotPeriod_s             = 36000.0,
        .GM_km3s2                = 282528.004209995,
        .orbit={.parent=&KspSystem::Kerbol,
                .LAN                 = 52.0,
                .LDN                 = 232.0,
                .argumentOfPeriapsis = 0.0,
                .meanAnomaly         = 0.100000001490116,
                .AP                  = 72212238.3872403,
                .PE                  = 65334882.2527597,
                .a_semi              = 68773560.32,
                .eccentricity        = 0.0500000007450581,
                .inclination         = 1.30400002002716,
                .epoch               = 0.0
        } };

    constexpr inline Body Laythe  {
        .name                    = "Laythe",
        .radius_km               = 500.0,
        .R_SOI_km                = 3723.64581113302,
        .surfaceGravity          = 7.84800011694431,
        .seaLevel_atm            = 0.6,
        .atmHeight_km            = 50.0,
        .atm_falloff_km          = 8.22121351014242,
        .sea_level_density_kgpm3 = 0.764571404126208,
        .rotPeriod_s             = 52980.8790593796,
        .GM_km3s2                = 1962.00002923608,
        .orbit={.parent=&KspSystem::Jool,
                .LAN                 = 0.0,
                .LDN                 = 180.0,
                .argumentOfPeriapsis = 0.0,
                .meanAnomaly         = 3.14000010490417,
                .AP                  = 27184.0,
                .PE                  = 27184.0,
                .a_semi              = 27184.0,
                .eccentricity        = 0.0,
                .inclination         = 0.0,
                .epoch               = 0.0
        } };

    constexpr inline Body Vall  {
        .name                    = "Vall",
        .radius_km               = 300.0,
        .R_SOI_km                = 2406.40144479404,
        .surfaceGravity          = 2.30534999415279,
        .seaLevel_atm            = 0.0,
        .atmHeight_km            = 0.0,
        .atm_falloff_km          = 0.0,
        .sea_level_density_kgpm3 = 0.0,
        .rotPeriod_s             = 105962.088893924,
        .GM_km3s2                = 207.481499473751,
        .orbit={.parent=&KspSystem::Jool,
                .LAN                 = 0.0,
                .LDN                 = 180.0,
                .argumentOfPeriapsis = 0.0,
                .meanAnomaly         = 0.899999976158142,
                .AP                  = 43152.0,
                .PE                  = 43152.0,
                .a_semi              = 43152.0,
                .eccentricity        = 0.0,
                .inclination         = 0.0,
                .epoch               = 0.0
        } };

    constexpr inline Body Tylo  {
        .name                    = "Tylo",
        .radius_km               = 600.0,
        .R_SOI_km                = 10856.5183683586,
        .surfaceGravity          = 7.84800011694431,
        .seaLevel_atm            = 0.0,
        .atmHeight_km            = 0.0,
        .atm_falloff_km          = 0.0,
        .sea_level_density_kgpm3 = 0.0,
        .rotPeriod_s             = 211926.35802123,
        .GM_km3s2                = 2825.28004209995,
        .orbit={.parent=&KspSystem::Jool,
                .LAN                 = 0.0,
                .LDN                 = 180.0,
                .argumentOfPeriapsis = 0.0,
                .meanAnomaly         = 3.14000010490417,
                .AP                  = 68500.0,
                .PE                  = 68500.0,
                .a_semi              = 68500.0,
                .eccentricity        = 0.0,
                .inclination         = 0.025000000372529,
                .epoch               = 0.0
        } };

    constexpr inline Body Bop  {
        .name                    = "Bop",
        .radius_km               = 65.0,
        .R_SOI_km                = 1221.06086284253,
        .surfaceGravity          = 0.588599986843765,
        .seaLevel_atm            = 0.0,
        .atmHeight_km            = 0.0,
        .atm_falloff_km          = 0.0,
        .sea_level_density_kgpm3 = 0.0,
        .rotPeriod_s             = 544507.428516654,
        .GM_km3s2                = 2.48683494441491,
        .orbit={.parent=&KspSystem::Jool,
                .LAN                 = 10.0,
                .LDN                 = 190.0,
                .argumentOfPeriapsis = 25.0,
                .meanAnomaly         = 0.899999976158142,
                .AP                  = 158697.499923408,
                .PE                  = 98302.5000765919,
                .a_semi              = 128500.0,
                .eccentricity        = 0.234999999403954,
                .inclination         = 15.0,
                .epoch               = 0.0
        } };

    constexpr inline Body Pol  {
        .name                    = "Pol",
        .radius_km               = 44.0,
        .R_SOI_km                = 1042.13889230178,
        .surfaceGravity          = 0.37278,
        .seaLevel_atm            = 0.0,
        .atmHeight_km            = 0.0,
        .atm_falloff_km          = 0.0,
        .sea_level_density_kgpm3 = 0.0,
        .rotPeriod_s             = 901902.623531173,
        .GM_km3s2                = 0.72170208,
        .orbit={.parent=&KspSystem::Jool,
                .LAN                 = 2.0,
                .LDN                 = 182.0,
                .argumentOfPeriapsis = 15.0,
                .meanAnomaly         = 0.899999976158142,
                .AP                  = 210624.2065,
                .PE                  = 149155.7935,
                .a_semi              = 179890.0,
                .eccentricity        = 0.17085,
                .inclination         = 4.25,
                .epoch               = 0.0
        } };

    constexpr inline Body Eeloo  {
        .name                    = "Eeloo",
        .radius_km               = 210.0,
        .R_SOI_km                = 119082.941647812,
        .surfaceGravity          = 1.68732005730271,
        .seaLevel_atm            = 0.0,
        .atmHeight_km            = 0.0,
        .atm_falloff_km          = 0.0,
        .sea_level_density_kgpm3 = 0.0,
        .rotPeriod_s             = 19460.0,
        .GM_km3s2                = 74.4108145270496,
        .orbit={.parent=&KspSystem::Kerbol,
                .LAN                 = 50.0,
                .LDN                 = 230.0,
                .argumentOfPeriapsis = 260.0,
                .meanAnomaly         = 3.14000010490417,
                .AP                  = 113549713.2,
                .PE                  = 66687926.8,
                .a_semi              = 90118820.0,
                .eccentricity        = 0.26,
                .inclination         = 6.15,
                .epoch               = 0.0
        } };
}

#endif // KSPCONSTANTS_H
