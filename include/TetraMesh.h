//
//  TetraMesh.h
//
//  Created by Petros Kataras on 2/10/16.
//
//

#ifndef TetraMesh_hpp
#define TetraMesh_hpp

#include "cinder/Cinder.h"
#include "cinder/gl/gl.h"

#include "TetrahedronTopology.h"
#include "TriMeshLoader.h"
#include "Octree.h"

#include "CGALTetrahedralize.h"
#include "CGALUtils.h"

namespace tetra {

struct DrawArraysIndirectCommand{
    GLuint vertexCount;
    GLuint instanceCount;
    GLuint firstVertex;
    GLuint baseInstance;
};
  
using namespace ci;

using TetraTopologyRef = std::shared_ptr<TetraTools::TetrahedronTopology>;
using TriangleTopologyRef = std::shared_ptr<TetraTools::TriangleTopology>;
using CGALTetrahedralizeRef = std::shared_ptr<CGALTetrahedralize::CGALTetrahedralize>;
using TetraBatchRefVec = std::vector<gl::BatchRef>;
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
        void draw();
        
        void multiDrawIndirect();
        const TetraTopologyRef& getTopology() const { return mTopology; }
        const gl::BatchRef& getMDIBatch() const { return mTetraMDI; }
        const gl::GlslProgRef& getMDIGlsl() const { return mGlslMDI; }
    private:
        void generateTetraBatches( const TetraTopologyRef &tetraTopology );
    
        TriangleTopologyRef loadSurface( const std::string &filename );
    
        TetraTopologyRef generateTetrasFromSurface( const TriangleTopologyRef &triMesh,
                                                    const double cellSize,
                                                    const double facetAngle,
                                                    const double facetSize,
                                                    const double facetDistance,
                                                    const double cellRadiusEdgeRatio );
        TetraTopologyRef mTopology;
        TriangleTopologyRef mSurface;
        TetraBatchRefVec mTetraBatches;
        gl::BatchRef mTetraMDI;
        std::vector<vec3> mVertices;
        std::vector<vec3> mNormals;
        std::vector<vec3> mCentroids;
        std::vector<mat4> mModelMatrices;
        gl::GlslProgRef mGlslMDI;
        gl::VboRef mIndirectBuffer;
        gl::VboRef mModelMatricesVbo;
};

} // namespace Tetra

#endif /* TetraMesh_h */
