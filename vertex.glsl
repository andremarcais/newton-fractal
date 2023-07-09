#version 420 core

uniform mat4 viewTrans;
in vec3 vertPos;
out mat2 z0;

void main()
{
    gl_Position = vec4(vertPos, 1.0);
    vec4 pos = viewTrans * vec4(vertPos, 1.0);
    z0 = mat2(pos.xy, -pos.y, pos.x);
}
