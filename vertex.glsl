#version 420 core

in vec3 vertPos;
in mat2 vertRoot;
out mat2 root;
out mat2 z0;

void main()
{
    gl_Position = vec4(vertPos, 1.0);
    root = vertRoot;
    z0 = 0.1*mat2(vertPos.xy, -vertPos.y, vertPos.x);
}
