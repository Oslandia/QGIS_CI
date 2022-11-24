#version 330

in vec3 worldPosition;
in vec3 worldNormal;
in vec2 texCoord;

uniform vec3 eyePosition;
uniform vec4 ka;
uniform vec4 ks;
uniform float shininess;
uniform sampler2D diffuseTexture;
uniform float opacity;

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

    vec4 diffuseTextureColor = vec4(texture(diffuseTexture, texCoord).rgb, opacity);
    vec3 worldView = normalize(eyePosition - worldPosition);
    fragColor = phongFunction(ka, diffuseTextureColor, ks, shininess, worldPosition, worldView, worldNormal);
}
