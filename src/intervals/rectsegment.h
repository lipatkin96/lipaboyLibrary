#ifndef RECTSEGMENT_H
#define RECTSEGMENT_H

#include "../maths/point2d.h"
#include "segment.h"

namespace lipaboy_lib {

//TODO: extend to template
class RectSegment
{
public:
    RectSegment(Segment<double> X, Segment<double> Y) : x(X), y(Y) {}
    bool in(lipaboy_lib::Point2D vec) const { return (x.in(vec.x()) && y.in(vec.y())); }
    bool out(lipaboy_lib::Point2D vec) const { return !in(vec); }

    Segment<double> xSegment() const { return x; }
    Segment<double> ySegment() const { return y; }

protected:
    Segment<double> x;
    Segment<double> y;
};

}

#endif // RECTSEGMENT_H
