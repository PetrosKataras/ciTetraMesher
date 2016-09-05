//
//  TetraMesh.h
//
//  Created by Petros Kataras on 2/10/16.
//
//

#ifndef TetraMesh_hpp
#define TetraMesh_hpp

#include "cinder/Cinder.h"

#include "TetrahedronTopology.h"
#include "TriMeshLoader.h"

#include "CGALTetrahedralize.h"
#include "CGALUtils.h"
#include "cinder/Utilities.h"

namespace tetra {

using namespace ci;

using TetraTopologyRef = std::shared_ptr<TetraTools::TetrahedronTopology>;
using TriangleTopologyRef = std::shared_ptr<TetraTools::TriangleTopology>;
using CGALTetrahedralizeRef = std::shared_ptr<CGALTetrahedralize>;
using TetraMeshRef = std::shared_ptr<class TetraMesh>;

class TetraMesh {
    public:
        static TetraMeshRef create( const fs::path& path,
                                    const double cellSize,
                                    const double facetAngle,
                                    const double facetSize,
                                    const double facetDistance,
                                    const double cellRadiusEdgeRatio )
                                { return TetraMeshRef( new TetraMesh( path, cellSize, facetAngle, facetSize, facetDistance, cellRadiusEdgeRatio ) ); }
    
        TetraMesh( const fs::path& path,
                    const double cellSize,
                    const double facetAngle,
                    const double facetSize,
                    const double facetDistance,
                    const double cellRadiusEdgeRatio );
    
        void update();
        const TetraTopologyRef& getTopology() const { return mTopology; }
    private:
        TriangleTopologyRef loadSurface( const std::string &filename );
    
        TetraTopologyRef generateTetrasFromSurface( const TriangleTopologyRef &triMesh,
                                                    const double cellSize,
                                                    const double facetAngle,
                                                    const double facetSize,
                                                    const double facetDistance,
                                                    const double cellRadiusEdgeRatio );
        TetraTopologyRef mTopology;
        TriangleTopologyRef mSurface;
        std::vector<vec3> mVertices;
        std::vector<vec3> mNormals;
        std::vector<vec3> mCentroids;
};

} // namespace Tetra

#endif /* TetraMesh_h */
