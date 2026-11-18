#version 120

uniform sampler1D transforms;
uniform int numTransforms;

attribute vec2 position;
attribute vec2 texCoords;
attribute float spriteNumber;

varying vec2 texCoordsOut;

vec2 cameraPos(vec2 worldPos) {
    return worldPos / vec2(1280.0, 720.0) * 2.0 - 1.0;
}

void main() {
    // sample at the texel centre so NEAREST picks exactly this sprite's entry
    vec4 t = texture1DLod(transforms, (spriteNumber + 0.5) / float(numTransforms), 0.0);
    vec2 scale = t.xy;
    vec2 translation = t.zw;

    vec2 world = position * scale + translation;  // same order as BasicTransform::applyPoint
    gl_Position = vec4(cameraPos(world), 0.0, 1.0);
    texCoordsOut = vec2(texCoords.x, 1.0 - texCoords.y);
}