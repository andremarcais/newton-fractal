#version 420 core

const float pi = 3.1415926535897932384626433832795;
const float tau = 2*pi;
const mat2 U = mat2(1.0, 0.0, 0.0, 1.0);
const mat2 I = mat2(0.0, 1.0, -1.0, 0.0);
in mat2 z0;
out vec4 FragColor;

// https://gamedev.stackexchange.com/questions/59797/glsl-shader-change-hue-saturation-brightness
vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

mat2 conj(mat2 z) {
    return mat2(z[0][0], -z[0][1], -z[1][0], z[1][1]);
}

float abs2(mat2 z) {
    return (z*conj(z))[0][0];
}

mat2 inv(mat2 z) {
    return conj(z)/abs2(z);
}

mat2 func(mat2 z) {
    return z*z*z*z - U;
}

mat2 deriv(mat2 z) {
    return 4*z*z*z;
}

mat2 newton(mat2 z) {
    for (int i = 0; i < 100; ++i) {
        z = z - func(z)*inv(deriv(z));
    }
    return z;
}

mat2 mandelbrot(mat2 c) {
    mat2 z = 0*U;
    for (int i = 0; i < 200; ++i) {
        z = z*z + c;
    }
    return z;
}

void main()
{
    mat2 z = newton(z0);
    // FragColor = vec4(z[0], 0.0, 1.0);
    FragColor = vec4(hsv2rgb(vec3(atan(z[0].y, z[0].x)/tau, 0.7, 0.8)), 1.0);
    // FragColor = vec4(5/abs2(mandelbrot(z0)), 0.0, 0.0, 1.0);
}
