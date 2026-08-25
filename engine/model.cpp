#include "engine/model.h"

namespace arctic {

MeshVertexFormat MeshFormatPosNormUv() {
  MeshVertexFormat format;
  format.AddElement("vPosition", 3, kRMVEDT_Float, false);
  format.AddElement("vNormal", 3, kRMVEDT_Float, false);
  format.AddElement("vTexCoord", 2, kRMVEDT_Float, false);
  return format;
}

}  // namespace arctic
