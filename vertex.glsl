#version 420 core

uniform dmat4 viewTrans;
in vec3 vertPos;
out vec2 screenPos;

void main() {
    gl_Position = vec4(vertPos, 1.0);
    screenPos = vertPos.xy;
}
