#version 330 core
out vec2 texCoord;

void main() {
    texCoord = vec2((gl_VertexID == 2) ? 2.0 : 0.0,
                    (gl_VertexID == 1) ? 2.0 : 0.0);
    gl_Position = vec4((texCoord - 1.0) * 2.0, 0.0, 1.0);
}