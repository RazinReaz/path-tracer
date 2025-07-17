#version 330 core
out vec2 texCoord;

void main() {
    const vec2 pos[3] = vec2[3](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    gl_Position = vec4(pos[gl_VertexID], 0.0, 1.0);
    texCoord = (gl_Position.xy + 1.0) * 0.5;
}