uniform sampler2D textureData;

// in vec4 transformedPosition;
in vec2 interpolatedTextureCoordinates;

out vec4 color;

void main() {
    color = texture(textureData, interpolatedTextureCoordinates).rgba;
}
