#ifndef CIRCLE_H
#define CIRCLE_H

struct Circle
{
    float x, y;
    double t0;
    float radius;   // main circle radius in UV
    float falloff;  // radial falloff exponent
    float intensity; // overall intensity/alpha multiplier
};


const int kMaxCircles = 128;
const double kCircleLife = 1; // seconds



#endif