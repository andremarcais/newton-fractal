#version 420 core

#define REAL double
#define COMPLEX dmat2

const REAL pi = 3.1415926535897932384626433832795;
const REAL tau = 2*pi;
const COMPLEX U = COMPLEX(1.0, 0.0, 0.0, 1.0);
const COMPLEX I = COMPLEX(0.0, 1.0, -1.0, 0.0);

uniform dmat4 viewTrans;
uniform COMPLEX poly[4];

in vec2 screenPos;
out vec4 FragColor;

// https://gamedev.stackexchange.com/questions/59797/glsl-shader-change-hue-saturation-brightness
dvec3 hsv2rgb(dvec3 c) {
    dvec4 K = dvec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    dvec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

COMPLEX conj(COMPLEX z) {
    return COMPLEX(z[0][0], -z[0][1], -z[1][0], z[1][1]);
}

REAL abs2(COMPLEX z) {
    return (z*conj(z))[0][0];
}

COMPLEX inv(COMPLEX z) {
    return conj(z)/abs2(z);
}

COMPLEX func(COMPLEX z) {
    return z*z*z*z - U;
}

COMPLEX deriv(COMPLEX z) {
    return 4*z*z*z;
}

COMPLEX newton(COMPLEX z) {
    for (int i = 0; i < 200; ++i) {
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
    // FragColor = vec4(newton(z0)[0] + 0.5, 0.0, 1.0);
    FragColor = mandelbrot_color(z0);
}
