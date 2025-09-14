#version 460 core
// Full-screen triangle generated from gl_VertexID.
// Pass normalized UV (0..1) to the fragment shader.
out vec2 vUV;
void main() {
    // A full-screen triangle that covers the viewport.
    const vec2 verts[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    gl_Position = vec4(verts[gl_VertexID], 0.0, 1.0);
    vUV = gl_Position.xy * 0.5 + 0.5; // convert clip-space (-1..1) to UV (0..1)
}
