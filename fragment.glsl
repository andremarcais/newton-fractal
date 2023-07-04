#version 420 core

in mat2 root;
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
    return z*z*z + z*z + z + 1;
}

mat2 deriv(mat2 z) {
    return 3*z*z + 2*z + 1;
}

mat2 newton(mat2 z0) {
    mat2 z = z0;
    for (int i = 0; i < 10; ++i) {
        z = z - func(z)*inv(deriv(z));
    }
    return z;
}

void main()
{
    //gl_FragDepth = 0.0;
    gl_FragDepth = root[0][0];
    //FragColor = vec4(newton(z0)[0], 0.0, 1.0);
    FragColor = vec4((z0*z0)[0], 0.0, 1.0);
}
