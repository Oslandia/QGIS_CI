bool isInsideBoundingBox(const in vec3 worldPos,
		         const in bool boundingBoxEnabled,
		         const in vec3 boundingBoxMin,
		         const in vec3 boundingBoxMax)
{
  if (boundingBoxEnabled &&
      (worldPos.x <= boundingBoxMin.x || worldPos.x >= boundingBoxMax.x ||
       worldPos.y <= boundingBoxMin.y || worldPos.y >= boundingBoxMax.y ||
       worldPos.z <= boundingBoxMin.z || worldPos.z >= boundingBoxMax.z))
    {
      return false;
    }

  return true;
}
