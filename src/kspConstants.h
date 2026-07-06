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
    const float surfaceGravity;
    const float seaLevel_atm; // in atm
    const float atmHeight_km;
    const float atm_falloff_km; // Approximate atm scale parameter
    const float sea_level_density_kgpm3;
    const float rotPeriod_s;
    const Orbit orbit;

    inline float GM() const {
        return (surfaceGravity / 1000.0) * radius_km * radius_km;
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
        .radius_km               = 261600.0f,
        .surfaceGravity          = 17.14f,
        .seaLevel_atm            = 0.157908f,
        .atmHeight_km            = 600.0f,
        .atm_falloff_km          = 0.0f,
        .sea_level_density_kgpm3 = 0.0f,
        .rotPeriod_s             = 432000.0f,
        .orbit                   = {} };
    constexpr inline Body Moho   { 
        .name                    = "Moho",
        .radius_km               = 250.0f,
        .surfaceGravity          = 2.7f,
        .seaLevel_atm            = 0.0f,
        .atmHeight_km            = 0.0f,
        .atm_falloff_km          = 0.0f,
        .sea_level_density_kgpm3 = 0.0f,
        .rotPeriod_s             = 1210000,
        .orbit={.refBody=&KspSystem::Kerbol,
                .LAN                 = 70.0f,
                .LDN                 = 250.0f,
                .argumentOfPeriapsis = 15.0f,
                .meanAnomaly         = 3.14f,
                .AP                  = 6315765.965,
                .PE                  = 4210510.643,
                .a_semi              = 5263138.304,
                .eccentricity        = 0.2f,
                .inclination         = 7.0f,
                .epoch               = 0.0f} };
    constexpr inline Body Eve    { 
        .name                    = "Eve",
        .radius_km               = 700.0f,
        .surfaceGravity          = 16.7f,
        .seaLevel_atm            = 5.0f,
        .atmHeight_km            = 90.0f,
        .atm_falloff_km          = 8.6720f,
        .sea_level_density_kgpm3 = 6.23837138885624f,
        .rotPeriod_s             = 80500,
        .orbit={.refBody=&KspSystem::Kerbol,
                .LAN                 = 15.0f,
                .LDN                 = 195.0f,
                .argumentOfPeriapsis = 0.0f,
                .meanAnomaly         = 3.14f,
                .AP                  = 9931011.389,
                .PE                  = 9734357.699,
                .a_semi              = 9832684.544,
                .eccentricity        = 0.01f,
                .inclination         = 2.1f,
                .epoch               = 0.0f} };
    constexpr inline Body Gilly  { 
        .name                    = "Gilly ",
        .radius_km               = 13.0f,
        .surfaceGravity          = 0.049f,
        .seaLevel_atm            = 0.0f,
        .atmHeight_km            = 0.0f,
        .atm_falloff_km          = 0.0f,
        .sea_level_density_kgpm3 = 0.0f,
        .rotPeriod_s             = 28255,
        .orbit={.refBody=&KspSystem::Eve,   
                .LAN                 = 80.0f,
                .LDN                 = 260.0f,
                .argumentOfPeriapsis = 10.0f,
                .meanAnomaly         = 0.9f,
                .AP                  = 48825.0,
                .PE                  = 14175.0,
                .a_semi              = 31500.0,
                .eccentricity        = 0.55f,
                .inclination         = 12.0f,
                .epoch               = 0.0f} };
    constexpr inline Body Kerbin { 
        .name                    = "Kerbin",
        .radius_km               = 600.0f,
        .surfaceGravity          = 9.81f,
        .seaLevel_atm            = 1.0f,
        .atmHeight_km            = 70.0f,
        .atm_falloff_km          = 5.9235f,
        .sea_level_density_kgpm3 = 1.22497705725583f,
        .rotPeriod_s             = 21549.4251830898,
        .orbit={.refBody=&KspSystem::Kerbol,
                .LAN                 = 0.0f,
                .LDN                 = 180.0f,
                .argumentOfPeriapsis = 0.0f,
                .meanAnomaly         = 3.14f,
                .AP                  = 13599840.256,
                .PE                  = 13599840.256,
                .a_semi              = 13599840.256,
                .eccentricity        = 0.0f,
                .inclination         = 0.0f,
                .epoch               = 0.0f} };
    constexpr inline Body Mun    { 
        .name                    = "Mun",
        .radius_km               = 200.0f,
        .surfaceGravity          = 1.63f,
        .seaLevel_atm            = 0.0f,
        .atmHeight_km            = 0.0f,
        .atm_falloff_km          = 0.0f,
        .sea_level_density_kgpm3 = 0.0f,
        .rotPeriod_s             = 138984.376574476,
        .orbit={.refBody=&KspSystem::Kerbin,
                .LAN                 = 0.0f,
                .LDN                 = 180.0f,
                .argumentOfPeriapsis = 0.0f,
                .meanAnomaly         = 1.7f,
                .AP                  = 12000.0,
                .PE                  = 12000.0,
                .a_semi              = 12000.0,
                .eccentricity        = 0.0f,
                .inclination         = 0.0f,
                .epoch               = 0.0f} };
    constexpr inline Body Minmus { 
        .name                    = "Minmus",
        .radius_km               = 60.0f,
        .surfaceGravity          = 0.491f,
        .seaLevel_atm            = 0.0f,
        .atmHeight_km            = 0.0f,
        .atm_falloff_km          = 0.0f,
        .sea_level_density_kgpm3 = 0.0f,
        .rotPeriod_s             = 40400,
        .orbit={.refBody=&KspSystem::Kerbin,
                .LAN                 = 78.0f,
                .LDN                 = 258.0f,
                .argumentOfPeriapsis = 38.0f,
                .meanAnomaly         = 0.9f,
                .AP                  = 47000.0,
                .PE                  = 47000.0,
                .a_semi              = 47000.0,
                .eccentricity        = 0.0f,
                .inclination         = 6.0f,
                .epoch               = 0.0f} };
    constexpr inline Body Duna   { 
        .name                    = "Duna",
        .radius_km               = 320.0f,
        .surfaceGravity          = 2.94f,
        .seaLevel_atm            = 0.0666667f,
        .atmHeight_km            = 50.0f,
        .atm_falloff_km          = 6.9421f,
        .sea_level_density_kgpm3 = 0.149935108881759f,
        .rotPeriod_s             = 65517.859375,
        .orbit={.refBody=&KspSystem::Kerbol,
                .LAN                 = 135.5f,
                .LDN                 = 315.5f,
                .argumentOfPeriapsis = 0.0f,
                .meanAnomaly         = 3.14f,
                .AP                  = 21783189.181,
                .PE                  = 19669121.346,
                .a_semi              = 20726155.264,
                .eccentricity        = 0.051f,
                .inclination         = 0.06f,
                .epoch               = 0.0f} };
    constexpr inline Body Ike    { 
        .name                    = "Ike",
        .radius_km               = 130.0f,
        .surfaceGravity          = 1.1f,
        .seaLevel_atm            = 0.0f,
        .atmHeight_km            = 0.0f,
        .atm_falloff_km          = 0.0f,
        .sea_level_density_kgpm3 = 0.0f,
        .rotPeriod_s             = 65517.8621348081,
        .orbit={.refBody=&KspSystem::Duna,  
                .LAN                 = 0.0f,
                .LDN                 = 180.0f,
                .argumentOfPeriapsis = 0.0f,
                .meanAnomaly         = 1.7f,
                .AP                  = 3296.0,
                .PE                  = 3104.0,
                .a_semi              = 3200.0,
                .eccentricity        = 0.03f,
                .inclination         = 0.2f,
                .epoch               = 0.0f} };
    constexpr inline Body Dres   { 
        .name                    = "Dres",
        .radius_km               = 138.0f,
        .surfaceGravity          = 1.13f,
        .seaLevel_atm            = 0.0f,
        .atmHeight_km            = 0.0f,
        .atm_falloff_km          = 0.0f,
        .sea_level_density_kgpm3 = 0.0f,
        .rotPeriod_s             = 34800,
        .orbit={.refBody=&KspSystem::Kerbol,
                .LAN                 = 280.0f,
                .LDN                 = 100.0f,
                .argumentOfPeriapsis = 90.0f,
                .meanAnomaly         = 3.14f,
                .AP                  = 46761053.692,
                .PE                  = 34917642.714,
                .a_semi              = 40839348.203,
                .eccentricity        = 0.145f,
                .inclination         = 5.0f,
                .epoch               = 0.0f} };
    constexpr inline Body Jool   { 
        .name                    = "Jool",
        .radius_km               = 6000.0f,
        .surfaceGravity          = 7.85f,
        .seaLevel_atm            = 15.0f,
        .atmHeight_km            = 138.2f,
        .atm_falloff_km          = 10.0f,
        .sea_level_density_kgpm3 = 0.0f,
        .rotPeriod_s             = 36000.0f,
        .orbit={.refBody=&KspSystem::Kerbol,
                .LAN                 = 52.0f,
                .LDN                 = 232.0f,
                .argumentOfPeriapsis = 0.0f,
                .meanAnomaly         = 0.1f,
                .AP                  = 72212238.336,
                .PE                  = 65334882.304,
                .a_semi              = 68773560.320,
                .eccentricity        = 0.05f,
                .inclination         = 1.304f,
                .epoch               = 0.0f} };
    constexpr inline Body Laythe { 
        .name                    = "Laythe",
        .radius_km               = 500.0f,
        .surfaceGravity          = 7.85f,
        .seaLevel_atm            = 0.6f,
        .atmHeight_km            = 50.0f,
        .atm_falloff_km          = 8.2212f,
        .sea_level_density_kgpm3 = 0.764571404126208f,
        .rotPeriod_s             = 52980.8790593796,
        .orbit={.refBody=&KspSystem::Jool,  
                .LAN                 = 0.0f,
                .LDN                 = 180.0f,
                .argumentOfPeriapsis = 0.0f,
                .meanAnomaly         = 3.14f,
                .AP                  = 27184.0,
                .PE                  = 27184.0,
                .a_semi              = 27184.0,
                .eccentricity        = 0.0f,
                .inclination         = 0.0f,
                .epoch               = 0.0f} };
    constexpr inline Body Vall   { 
        .name                    = "Vall",
        .radius_km               = 300.0f,
        .surfaceGravity          = 2.31f,
        .seaLevel_atm            = 0.0f,
        .atmHeight_km            = 0.0f,
        .atm_falloff_km          = 0.0f,
        .sea_level_density_kgpm3 = 0.0f,
        .rotPeriod_s             = 105962.088893924,
        .orbit={.refBody=&KspSystem::Jool,  
                .LAN                 = 0.0f,
                .LDN                 = 180.0f,
                .argumentOfPeriapsis = 0.0f,
                .meanAnomaly         = 0.9f,
                .AP                  = 43152.0,
                .PE                  = 43152.0,
                .a_semi              = 43152.0,
                .eccentricity        = 0.0f,
                .inclination         = 0.0f,
                .epoch               = 0.0f} };
    constexpr inline Body Tylo   { 
        .name                    = "Tylo",
        .radius_km               = 600.0f,
        .surfaceGravity          = 7.85f,
        .seaLevel_atm            = 0.0f,
        .atmHeight_km            = 0.0f,
        .atm_falloff_km          = 0.0f,
        .sea_level_density_kgpm3 = 0.0f,
        .rotPeriod_s             = 211926.35802123,
        .orbit={.refBody=&KspSystem::Jool,  
                .LAN                 = 0.0f,
                .LDN                 = 180.0f,
                .argumentOfPeriapsis = 0.0f,
                .meanAnomaly         = 3.14f,
                .AP                  = 68500.0,
                .PE                  = 68500.0,
                .a_semi              = 68500.0,
                .eccentricity        = 0.0f,
                .inclination         = 0.025f,
                .epoch               = 0.0f} };
    constexpr inline Body Bop    { 
        .name                    = "Bop",
        .radius_km               = 65.0f,
        .surfaceGravity          = 0.589f,
        .seaLevel_atm            = 0.0f,
        .atmHeight_km            = 0.0f,
        .atm_falloff_km          = 0.0f,
        .sea_level_density_kgpm3 = 0.0f,
        .rotPeriod_s             = 544507.428516654,
        .orbit={.refBody=&KspSystem::Jool,  
                .LAN                 = 10.0f,
                .LDN                 = 190.0f,
                .argumentOfPeriapsis = 25.0f,
                .meanAnomaly         = 0.9f,
                .AP                  = 158697.5,
                .PE                  = 98302.5,
                .a_semi              = 128500.0,
                .eccentricity        = 0.235f,
                .inclination         = 15.0f,
                .epoch               = 0.0f} };
    constexpr inline Body Pol    { 
        .name                    = "Pol",
        .radius_km               = 44.0f,
        .surfaceGravity          = 0.373f,
        .seaLevel_atm            = 0.0f,
        .atmHeight_km            = 0.0f,
        .atm_falloff_km          = 0.0f,
        .sea_level_density_kgpm3 = 0.0f,
        .rotPeriod_s             = 901902.623531173,
        .orbit={.refBody=&KspSystem::Jool,  
                .LAN                 = 2.0f,
                .LDN                 = 182.0f,
                .argumentOfPeriapsis = 15.0f,
                .meanAnomaly         = 0.9f,
                .AP                  = 210624.207,
                .PE                  = 149156.394,
                .a_semi              = 179890.0,
                .eccentricity        = 0.17085f,
                .inclination         = 4.25f,
                .epoch               = 0.0f} };
    constexpr inline Body Eeloo  { 
        .name                    = "Eeloo",
        .radius_km               = 210.0f,
        .surfaceGravity          = 1.69f,
        .seaLevel_atm            = 0.0f,
        .atmHeight_km            = 0.0f,
        .atm_falloff_km          = 0.0f,
        .sea_level_density_kgpm3 = 0.0f,
        .rotPeriod_s             = 19460,
        .orbit={.refBody=&KspSystem::Kerbol,
                .LAN                 = 50.0f,
                .LDN                 = 230.0f,
                .argumentOfPeriapsis = 260.0f,
                .meanAnomaly         = 3.14f,
                .AP                  = 113549713.2,
                .PE                  = 66687926.8,
                .a_semi              = 90118820.0,
                .eccentricity        = 0.26f,
                .inclination         = 6.15f,
                .epoch               = 0.0f} };
}

#endif // KSPCONSTANTS_H
