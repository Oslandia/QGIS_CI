#version 150 core

uniform sampler2D tex0;

in vec3 worldPosition;
in vec2 UV;
out vec4 fragColor;

// bounding box parameters
uniform bool boundingBoxEnabled;
uniform vec3 boundingBoxMin;
uniform vec3 boundingBoxMax;

#pragma include boundingboxcut.inc.frag

void main(void)
{
  if (!isInsideBoundingBox(worldPosition, boundingBoxEnabled, boundingBoxMin, boundingBoxMax))
  {
      discard;
      return;
  }

  fragColor = texture(tex0, UV);

  if (fragColor.a < 0.5)
      discard;
}
