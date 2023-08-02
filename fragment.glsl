#version 420 core

const double pi = 3.1415926535897932384626433832795;
const double tau = 2*pi;
const dmat2 U = dmat2(1.0, 0.0, 0.0, 1.0);
const dmat2 I = dmat2(0.0, 1.0, -1.0, 0.0);

uniform dmat4 viewTrans;

in vec2 screenPos;
out vec4 FragColor;

// https://gamedev.stackexchange.com/questions/59797/glsl-shader-change-hue-saturation-brightness
dvec3 hsv2rgb(dvec3 c) {
    dvec4 K = dvec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    dvec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

dmat2 conj(dmat2 z) {
    return dmat2(z[0][0], -z[0][1], -z[1][0], z[1][1]);
}

double abs2(dmat2 z) {
    return (z*conj(z))[0][0];
}

dmat2 inv(dmat2 z) {
    return conj(z)/abs2(z);
}

dmat2 func(dmat2 z) {
    return z*z*z*z - U;
}

dmat2 deriv(dmat2 z) {
    return 4*z*z*z;
}

dmat2 newton(dmat2 z) {
    for (int i = 0; i < 200; ++i) {
        z = z - func(z)*inv(deriv(z));
    }
    return z;
}

dmat2 mandelbrot(dmat2 c) {
    dmat2 z = 0*U;
    for (int i = 0; i < 200; ++i) {
        z = z*z + c;
    }
    return z;
}

vec4 mandelbrot_color(dmat2 c) {
    dmat2 z = 0*U;
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
    dmat2 z0 = dmat2(pos.xy, -pos.y, pos.x);
    // FragColor = vec4(newton(z0)[0] + 0.5, 0.0, 1.0);
    FragColor = mandelbrot_color(z0);
}
