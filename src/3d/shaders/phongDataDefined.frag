#version 330

uniform vec3 eyePosition;
uniform float shininess;

in vec3 worldPosition;
in vec3 worldNormal;

in DataColor {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
} vs_in;

// bounding box parameters
uniform bool boundingBoxEnabled;
uniform vec3 boundingBoxMin;
uniform vec3 boundingBoxMax;

out vec4 fragColor;

#pragma include phong.inc.frag
#pragma include boundingboxcut.inc.frag

void main(void)
{
    if (!isInsideBoundingBox(worldPosition, boundingBoxEnabled, boundingBoxMin, boundingBoxMax))
    {
        discard;
        return;
    }

    vec3 worldView = normalize(eyePosition - worldPosition);
    fragColor = phongFunction(vs_in.ambient,vs_in.diffuse,vs_in.specular, 1.0, worldPosition, worldView, worldNormal);
}
