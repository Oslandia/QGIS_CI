#version 150

uniform vec4 lineColor;
uniform bool useTex;
uniform sampler2D tex0;

// bounding box parameters
uniform bool boundingBoxEnabled;
uniform vec3 boundingBoxMin;
uniform vec3 boundingBoxMax;

in VertexData{
    vec2 mTexCoord;
//	vec3 mColor;
} VertexIn;

in vec3 worldPosition;

out vec4 oColor;

#pragma include boundingboxcut.inc.frag

void main(void)
{
    if (!isInsideBoundingBox(worldPosition, boundingBoxEnabled, boundingBoxMin, boundingBoxMax))
    // if (worldPosition.x < 0.0)
    {
        discard;
        return;
    }

    if (!useTex)
    {
        // option 1: plain color
        oColor = lineColor;
    }
    else
    {
        // option 2: textured color
        oColor = texture(tex0, VertexIn.mTexCoord.xy );
    }

    //vec4 clr = texture( tex0, VertexIn.mTexCoord.xy );
    //oColor.rgb = VertexIn.mColor * clr.rgb;
    //oColor.a = clr.a;
}
