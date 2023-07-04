#version 420 core

const mat2 I = mat2(1.0, 0.0, 0.0, 1.0);
in mat2 z0;
out vec4 FragColor;

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
    return z*z*z*z - I;
}

mat2 deriv(mat2 z) {
    return 4*z*z*z;
}

mat2 newton(mat2 z) {
    for (int i = 0; i < 50; ++i) {
        z = z - func(z)*inv(deriv(z));
    }
    return z;
}

mat2 mandelbrot(mat2 c) {
    mat2 z = 0*I;
    for (int i = 0; i < 200; ++i) {
        z = z*z + c;
    }
    return z;
}

void main()
{
    //FragColor = vec4(newton(z0)[0], 0.0, 1.0);
    FragColor = vec4(5/abs2(mandelbrot(z0)), 0.0, 0.0, 1.0);
}
