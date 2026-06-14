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
};

struct Body {
    const float radius_km;
    const float surfaceGravity;
    const float seaLevel_atm; // in atm
    const float atmHeight_km;

    static double getPressureAtAltitude(const Body& body, float altitude) {
        if (altitude >= body.atmHeight_km) return 0.0f;
        return body.seaLevel_atm * std::exp(-altitude / (body.atmHeight_km / 5.0f));
    }
};

namespace KspSystem {
    inline constexpr Body Kerbin { .radius_km=600.0f,  .surfaceGravity=9.81f,   .seaLevel_atm=1.0f,  .atmHeight_km=70.0f };
    inline constexpr Body Mun    { .radius_km=200.0f,  .surfaceGravity=1.63f,   .seaLevel_atm=0.0f,  .atmHeight_km=0.0f };
    inline constexpr Body Minmus { .radius_km=60.0f,   .surfaceGravity=0.491f,  .seaLevel_atm=0.0f,  .atmHeight_km=0.0f };
    inline constexpr Body Moho   { .radius_km=250.0f,  .surfaceGravity=2.7f,    .seaLevel_atm=0.0f,  .atmHeight_km=0.0f };
    inline constexpr Body Eve    { .radius_km=700.0f,  .surfaceGravity=16.7f,   .seaLevel_atm=5.0f,  .atmHeight_km=90.0f };
    inline constexpr Body Gilly  { .radius_km=13.0f,   .surfaceGravity=0.049f,  .seaLevel_atm=0.0f,  .atmHeight_km=0.0f };
    inline constexpr Body Dres   { .radius_km=138.0f,  .surfaceGravity=1.13f,   .seaLevel_atm=0.0f,  .atmHeight_km=0.0f };
    inline constexpr Body Duna   { .radius_km=320.0f,  .surfaceGravity=2.94f,   .seaLevel_atm=0.0666667f, .atmHeight_km=50.0f };
    inline constexpr Body Ike    { .radius_km=130.0f,  .surfaceGravity=1.1f,    .seaLevel_atm=0.0f,  .atmHeight_km=0.0f };
    inline constexpr Body Laythe { .radius_km=500.0f,  .surfaceGravity=7.85f,   .seaLevel_atm=0.6f,  .atmHeight_km=50.0f };
    inline constexpr Body Vall   { .radius_km=300.0f,  .surfaceGravity=2.31f,   .seaLevel_atm=0.0f,  .atmHeight_km=0.0f };
    inline constexpr Body Tylo   { .radius_km=600.0f,  .surfaceGravity=7.85f,   .seaLevel_atm=0.0f,  .atmHeight_km=0.0f };
    inline constexpr Body Bop    { .radius_km=65.0f,   .surfaceGravity=0.589f,  .seaLevel_atm=0.0f,  .atmHeight_km=0.0f };
    inline constexpr Body Pol    { .radius_km=44.0f,   .surfaceGravity=0.373f,  .seaLevel_atm=0.0f,  .atmHeight_km=0.0f };
    inline constexpr Body Eeloo  { .radius_km=210.0f,  .surfaceGravity=1.69f,   .seaLevel_atm=0.0f,  .atmHeight_km=0.0f };
}

#endif // KSPCONSTANTS_H
