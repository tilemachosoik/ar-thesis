layout(binding = 0)
uniform sampler2D textureImage;

in vec2 textureCoordinates;

out vec4 color;

void main() {
    // vec4 col = texture(textureImage, textureCoordinates);
    // if (col.a > 0.) {
    //     color.rgb = col.rgb * col.a;
    //     color.a = 1.;
    // }
    // else
    //     color = vec4(1., 1., 1., 0.);
    color = texture(textureImage, textureCoordinates);
}
