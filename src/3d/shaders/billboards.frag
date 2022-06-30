#version 150 core

uniform sampler2D tex0;

in vec2 UV;
out vec4 fragColor;

uniform bool boundingBoxEnabled;

void main(void)
{
  if (boundingBoxEnabled && worldPosition.x < 0.0) {
    discard;
    return;
  }

  fragColor = texture(tex0, UV);

  if (fragColor.a < 0.5)
      discard;
}
