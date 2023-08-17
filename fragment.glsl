#version 420 core

#define REAL double
#define COMPLEX dmat2
#define MAX_DEGREE 4

const REAL pi = 3.1415926535897932384626433832795;
const REAL tau = 2*pi;
const COMPLEX U = COMPLEX(1.0, 0.0, 0.0, 1.0);
const COMPLEX I = COMPLEX(0.0, 1.0, -1.0, 0.0);

uniform dmat4 viewTrans;
uniform int mode;
uniform COMPLEX poly[MAX_DEGREE+1];

in vec2 screenPos;
out vec4 FragColor;

// https://gamedev.stackexchange.com/questions/59797/glsl-shader-change-hue-saturation-brightness
dvec3 hsv2rgb(dvec3 c) {
    dvec4 K = dvec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    dvec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

COMPLEX conj(COMPLEX z) {
    return transpose(z);
}

REAL abs2(COMPLEX z) {
    return (z*conj(z))[0][0];
}

COMPLEX inv(COMPLEX z) {
    return conj(z)/abs2(z);
}

COMPLEX func(COMPLEX z) {
    return poly[0] + poly[1]*z + poly[2]*z*z + poly[3]*z*z*z + poly[4]*z*z*z*z;
    // TODO does GPU not like for loop?
    //COMPLEX w = 0*U;
    //for (int k = MAX_DEGREE; k >= 0; --k) {
    //    w = poly[k] + z*w;
    //}
    //return w;
}

COMPLEX deriv(COMPLEX z) {
    return poly[1] + 2*poly[2]*z + 3*poly[3]*z*z + 4*poly[4]*z*z*z;
    // TODO does GPU not like for loop?
    //COMPLEX w = 0*U;
    //for (int k = MAX_DEGREE; k >= 1; --k) {
    //    w = k*poly[k] + z*w;
    //}
    //return w;
}

COMPLEX newton(COMPLEX z) {
    for (int i = 0; i < 40; ++i) {
        z = z - func(z)*inv(deriv(z));
    }
    return z;
}

COMPLEX mandelbrot(COMPLEX c) {
    COMPLEX z = 0*U;
    for (int i = 0; i < 200; ++i) {
        z = z*z + c;
    }
    return z;
}

vec4 mandelbrot_color(COMPLEX c) {
    COMPLEX z = 0*U;
    for (int i = 0; i < 800; ++i) {
        z = z*z + c;
        if (abs2(z) > 4) {
            return vec4(hsv2rgb(vec3(i/100.0, 0.5, 1.0)), 1.0);
        }
    }
    return vec4(0.0, 0.0, 0.0, 1.0);
}

void main() {
    dvec4 pos = viewTrans * dvec4(screenPos, 0.0, 1.0);
    COMPLEX z0 = COMPLEX(pos.xy, -pos.y, pos.x);
    switch (mode) {
    case 0:
        FragColor = vec4(func(z0)[0] + 0.5, 0.0, 1.0);
        break;
    case 1:
        FragColor = vec4(newton(z0)[0] + 0.5, 0.0, 1.0);
        break;
    case 2:
        FragColor = mandelbrot_color(z0);
        break;
    }
}
