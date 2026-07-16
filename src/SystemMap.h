#ifndef SYSTEM_MAP_H
#define SYSTEM_MAP_H

struct Mission;

class SystemMap {
public:
    bool show = false;

    void render(const Mission& mission);

private:
    float azimuth = 0.4f;
    float elevation = 0.6f;
    float distance = 8000000.0f;
    float fov = 45.0f;
};

#endif // SYSTEM_MAP_H
