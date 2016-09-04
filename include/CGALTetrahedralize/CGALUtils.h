//
//  CGALUtils.h
//  Tetra
//
//  Created by Petros Kataras on 2/9/16.
//
//

#ifndef CGALUtils_h
#define CGALUtils_h

#include <vector>
#include "cinder/Vector.h"

namespace CGALUtils {
   void calculateAddTetraNormals( const std::vector<cinder::vec3>& verts, std::vector<cinder::vec3>& normals );
}

#endif /* CGALUtils_h */
