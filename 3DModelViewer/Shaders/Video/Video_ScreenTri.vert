#version 460
layout(location = 0) out vec2 outTexcoord;
layout(location = 1) out vec4 outPos;

void main()
{
    outTexcoord.x = (gl_VertexIndex == 2) ? 2.0 : 0.0;
    outTexcoord.y = (gl_VertexIndex == 1) ? 2.0 : 0.0;

    outPos = vec4(outTexcoord * vec2(2.0, -2.0) + vec2(-1.0, 1.0), 0.0, 1.0);
    outPos.y *= -1.0f;

    gl_Position = outPos;
}