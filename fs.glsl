#version 330 core

out vec4 color;

uniform bool Selected;
uniform vec3 Color;

uniform sampler2D Texture;
in vec2 TexCoord;
uniform bool TextureEnabled = false;
uniform float DisappearingTexture = 1.0f;

void main()
{
    if (TextureEnabled)
	color = mix(vec4(Color, 1.0f), texture2D(Texture, TexCoord), DisappearingTexture);
    else
	color = vec4(Color, 1.0f);

    if (Selected)
	color -= 0.2f;

}
