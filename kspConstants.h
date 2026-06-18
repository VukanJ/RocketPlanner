#ifndef KSPCONSTANTS_H
#define KSPCONSTANTS_H

#include <cmath>

namespace Constants {
    // Densities in tons/unit liter
    constexpr inline double LiquidFuelDensity     = 0.005;
    constexpr inline double OxidizerDensity       = 0.005;
    constexpr inline double MonoPropellantDensity = 0.004;
    constexpr inline double SolidFuelDensity      = 0.0075;
    constexpr inline double XenonGasDensity       = 0.0001;

    constexpr inline double mk1_area_m2 = M_PI * 1.2 * 1.2;
    constexpr inline double mk2_area_m2 = M_PI * 1.9 * 1.9;
    constexpr inline double mk3_area_m2 = M_PI * 2.5 * 2.5;

    constexpr inline double atmPascal = 101325.0;
    constexpr inline double RGas_J_kgK = 287.053; // Specific gas constant 
};

struct Body {
    const float radius_km;
    const float surfaceGravity;
    const float seaLevel_atm; // in atm
    const float atmHeight_km;
    const float atm_falloff_km; // Approximate atm scale parameter
    const float sea_level_density_kgpm3;
    const float rotPeriod_s;

    inline double GM() const {
        return (surfaceGravity / 1000.0) * radius_km * radius_km;
    }

    double getPressureAtAltitude_km(float altitude) const {
        if (altitude >= atmHeight_km) return 0.0f;
        return seaLevel_atm * std::exp(-altitude / atm_falloff_km);
    }
};


namespace KspSystem {
    constexpr inline Body Kerbin { .radius_km=600.0f, .surfaceGravity=9.81f,  .seaLevel_atm=1.0f,      .atmHeight_km=70.0f, .atm_falloff_km=5.9235f, .sea_level_density_kgpm3=1.22497705725583f,  .rotPeriod_s=21549.4251830898 };
    constexpr inline Body Eve    { .radius_km=700.0f, .surfaceGravity=16.7f,  .seaLevel_atm=5.0f,      .atmHeight_km=90.0f, .atm_falloff_km=8.6720f, .sea_level_density_kgpm3=6.23837138885624f,  .rotPeriod_s=80500 };
    constexpr inline Body Duna   { .radius_km=320.0f, .surfaceGravity=2.94f,  .seaLevel_atm=0.0666667f,.atmHeight_km=50.0f, .atm_falloff_km=6.9421f, .sea_level_density_kgpm3=0.149935108881759f, .rotPeriod_s=65517.859375 };
    constexpr inline Body Laythe { .radius_km=500.0f, .surfaceGravity=7.85f,  .seaLevel_atm=0.6f,      .atmHeight_km=50.0f, .atm_falloff_km=8.2212f, .sea_level_density_kgpm3=0.764571404126208f, .rotPeriod_s=52980.8790593796};

    constexpr inline Body Mun    { .radius_km=200.0f, .surfaceGravity=1.63f,  .seaLevel_atm=0.0f,      .atmHeight_km=0.0f,  .atm_falloff_km=0.0f,    .sea_level_density_kgpm3=0.0f, .rotPeriod_s=138984.376574476 };
    constexpr inline Body Minmus { .radius_km=60.0f,  .surfaceGravity=0.491f, .seaLevel_atm=0.0f,      .atmHeight_km=0.0f,  .atm_falloff_km=0.0f,    .sea_level_density_kgpm3=0.0f, .rotPeriod_s=40400 };
    constexpr inline Body Moho   { .radius_km=250.0f, .surfaceGravity=2.7f,   .seaLevel_atm=0.0f,      .atmHeight_km=0.0f,  .atm_falloff_km=0.0f,    .sea_level_density_kgpm3=0.0f, .rotPeriod_s=1210000 };
    constexpr inline Body Gilly  { .radius_km=13.0f,  .surfaceGravity=0.049f, .seaLevel_atm=0.0f,      .atmHeight_km=0.0f,  .atm_falloff_km=0.0f,    .sea_level_density_kgpm3=0.0f, .rotPeriod_s=28255 };
    constexpr inline Body Dres   { .radius_km=138.0f, .surfaceGravity=1.13f,  .seaLevel_atm=0.0f,      .atmHeight_km=0.0f,  .atm_falloff_km=0.0f,    .sea_level_density_kgpm3=0.0f, .rotPeriod_s=34800 };
    constexpr inline Body Ike    { .radius_km=130.0f, .surfaceGravity=1.1f,   .seaLevel_atm=0.0f,      .atmHeight_km=0.0f,  .atm_falloff_km=0.0f,    .sea_level_density_kgpm3=0.0f, .rotPeriod_s=65517.8621348081 };
    constexpr inline Body Vall   { .radius_km=300.0f, .surfaceGravity=2.31f,  .seaLevel_atm=0.0f,      .atmHeight_km=0.0f,  .atm_falloff_km=0.0f,    .sea_level_density_kgpm3=0.0f, .rotPeriod_s=105962.088893924 };
    constexpr inline Body Tylo   { .radius_km=600.0f, .surfaceGravity=7.85f,  .seaLevel_atm=0.0f,      .atmHeight_km=0.0f,  .atm_falloff_km=0.0f,    .sea_level_density_kgpm3=0.0f, .rotPeriod_s=211926.35802123 };
    constexpr inline Body Bop    { .radius_km=65.0f,  .surfaceGravity=0.589f, .seaLevel_atm=0.0f,      .atmHeight_km=0.0f,  .atm_falloff_km=0.0f,    .sea_level_density_kgpm3=0.0f, .rotPeriod_s=544507.428516654 };
    constexpr inline Body Pol    { .radius_km=44.0f,  .surfaceGravity=0.373f, .seaLevel_atm=0.0f,      .atmHeight_km=0.0f,  .atm_falloff_km=0.0f,    .sea_level_density_kgpm3=0.0f, .rotPeriod_s=901902.623531173 };
    constexpr inline Body Eeloo  { .radius_km=210.0f, .surfaceGravity=1.69f,  .seaLevel_atm=0.0f,      .atmHeight_km=0.0f,  .atm_falloff_km=0.0f,    .sea_level_density_kgpm3=0.0f, .rotPeriod_s=19460 };
}

#endif // KSPCONSTANTS_H
