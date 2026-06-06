#ifndef STAGE_H
#define STAGE_H

#include "parts.h"
#include <list>
class Stage {
public:

    bool hasPropulsion() const;

    double DeltaV() const;
    double TotalMass() const;
    double TWR() const;

    std::list<Part> all_parts;

    Part* attachAbove(Part* part, const PartProperty* attach);
    Part* attachBelow(Part* part, const PartProperty* attach);

    void setTop(const PartProperty* top);

    double mass_stage_above = 0.0;

    Part* Top = nullptr; // The topmost part of the stage. Connects to the next stage

};

#endif // STAGE_H
