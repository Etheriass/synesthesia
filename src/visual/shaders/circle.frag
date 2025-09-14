#version 460 core
in vec2 vUV;
out vec4 fragColor;

uniform int uCount;
uniform vec2 uPos[64];   // circle centers in UV (0..1)
uniform float uAge[64];  // elapsed time since spawn (seconds)
uniform float uLife;     // lifetime of a circle (seconds)

// Dotted rendering parameters
uniform float uRadius;       // default main circle radius in UV (used if per-circle not provided)
uniform float uDotSpacing;   // nominal spacing between dots in UV (radial)
uniform float uDotRadius;    // radius of each small dot in UV
// optional per-circle parameters
uniform float uRadiusArr[64];
uniform float uFalloffArr[64];
uniform float uIntensityArr[64];

// hash helpers for jitter
float hash1(float n) { return fract(sin(n) * 43758.5453123); }
float hash1(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123); }
vec2 hash2(vec2 p) { return vec2(hash1(p), hash1(p + 1.0)); }

const float PI = 3.14159265358979323846;

void main() {
    float alpha = 0.0;
    float dotEdge = uDotRadius * 0.35;

    for (int i = 0; i < uCount; ++i) {
        float age = uAge[i];
        if (age >= uLife) continue;

    float fade = clamp(1.0 - age / uLife, 0.0, 1.0);

        // relative position to circle center
        vec2 rel = vUV - uPos[i];
        float r = length(rel);
    // select per-circle radius if provided (fallback to uRadius)
    float circRadius = uRadius;
    if (uRadiusArr[i] > 0.0) circRadius = uRadiusArr[i];
    if (r > circRadius + uDotRadius) continue;

        // radial bin size
        float dr = max(1e-6, uDotSpacing);
        float kf = floor(r / dr);
    float r_center = (kf + 0.5) * dr;
        if (r_center < 1e-4) r_center = dr * 0.5;
        int k = int(kf);

        // number of angular cells for this ring (keep roughly uniform density)
        float circumference = 2.0 * PI * r_center;
        int N = int(max(1.0, floor(circumference / dr)));

        // fragment angle and angular cell
        float theta = atan(rel.y, rel.x);
        if (theta < 0.0) theta += 2.0 * PI;
        float dtheta = 2.0 * PI / float(N);
        int m = int(floor(theta / dtheta));

        // center angle of the cell
        float angle_center = (float(m) + 0.5) * dtheta;

        // jitter per cell using a hash of (k,m)
        vec2 h = hash2(vec2(float(k), float(m)));
        float jitterRad = (h.x - 0.5) * dr * 0.6;        // radial jitter
        float jitterAng = (h.y - 0.5) * dtheta * 0.4;   // angular jitter

        float finalR = r_center + jitterRad;
        float finalAngle = angle_center + jitterAng;

        vec2 dotCenter = vec2(cos(finalAngle), sin(finalAngle)) * finalR;

        float dDot = length(rel - dotCenter);
        float aDot = 1.0 - smoothstep(uDotRadius - dotEdge, uDotRadius + dotEdge, dDot);

    // radial falloff for density/visibility (stronger at center)
    float falloff = 1.4;
    if (uFalloffArr[i] > 0.0) falloff = uFalloffArr[i];
    float radialFactor = pow(clamp(1.0 - (r_center / max(circRadius, 1e-6)), 0.0, 1.0), falloff);
    // intensity multiplier per-circle
    float intensity = 1.0;
    if (uIntensityArr[i] > 0.0) intensity = uIntensityArr[i];
    aDot *= radialFactor * fade * intensity;

        alpha = max(alpha, aDot);
    }

    fragColor = vec4(vec3(1.0) * alpha, 1.0);
}
