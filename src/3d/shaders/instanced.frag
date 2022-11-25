#version 150 core

// copy of phong.frag from qt3d extras

uniform vec3 ka;                            // Ambient reflectivity
uniform vec3 kd;                            // Diffuse reflectivity
uniform vec3 ks;                            // Specular reflectivity
uniform float shininess;                    // Specular shininess factor
uniform float opacity;                      // Opacity

// bounding box parameters
uniform bool boundingBoxEnabled;
uniform vec3 boundingBoxMin;
uniform vec3 boundingBoxMax;

uniform vec3 eyePosition;

in vec3 worldPosition;
in vec3 worldNormal;

out vec4 fragColor;

#pragma include light.inc.frag
#pragma include boundingboxcut.inc.frag

void main()
{
    if (!isInsideBoundingBox(worldPosition, boundingBoxEnabled, boundingBoxMin, boundingBoxMax))
    {
        discard;
        return;
    }

    vec3 diffuseColor, specularColor;
    vec3 worldView = normalize(eyePosition - worldPosition);
    adsModel(worldPosition, worldNormal, worldView, shininess, diffuseColor, specularColor);
    fragColor = vec4( ka + kd * diffuseColor + ks * specularColor, opacity );
}
