#version 120

varying vec2 texCoordsOut;

// world: (0,0) bottom-left .. (1280,720) top-right  ->  clip space [-1,1]^2
vec2 cameraPos(vec2 worldPos) {
    return worldPos / vec2(1280.0, 720.0) * 2.0 - 1.0;
}

void main() {
    gl_Position = vec4(cameraPos(gl_Vertex.xy), 0.0, 1.0);
    // Our UVs use OpenGL's bottom-left origin, but SFML uploads image rows top-first,
    // so flip v to land on the right texel.
    texCoordsOut = vec2(gl_MultiTexCoord0.x, 1.0 - gl_MultiTexCoord0.y);
}