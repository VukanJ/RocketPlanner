#ifndef ORBIT_H
#define ORBIT_H

struct Body;

struct Orbit {
    const Body* refBody = nullptr;
    float LAN = 0;  // Longitude of ascending node
    float LDN = 0;  // Longitude of descending node
    float argumentOfPeriapsis = 0;
    float meanAnomaly = 0;
    double AP = 0;
    double PE = 0;
    double a_semi = 0;
    float eccentricity = 0;
    float inclination = 0;
    float epoch = 0; // Seconds elapsed at t = 0
};

#endif // ORBIT_H
