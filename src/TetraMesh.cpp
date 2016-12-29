//
//  TetraMesh.cpp
//  Tetra
//
//  Created by Petros Kataras on 2/10/16.
//
//

#include "TetraMesh.h"
#include "cinder/Log.h"
#include "cinder/app/App.h"

namespace tetra {

using namespace ci;
using namespace ci::app;

TetraMesh::TetraMesh( const fs::path& path,
                        const double cellSize,
                        const double facetAngle,
                        const double facetSize,
                        const double facetDistance,
                        const double cellRadiusEdgeRatio )
{
    generateTetrasFromSurface( loadSurface( path.string() ), cellSize, facetAngle, facetSize, facetDistance, cellRadiusEdgeRatio );
}

TriangleTopologyRef TetraMesh::loadSurface(const std::string& filename )
{
    TetraTools::TriMeshLoader loader;
    ci::Timer timer;
    timer.start();
    if( !loader.loadFile( filename ) ) {
        CI_LOG_E("Error loading file : " << filename );
        return nullptr;
    }

    mSurface = std::make_shared<TetraTools::TriangleTopology>();
    mSurface->Init( loader.GetVertices(), loader.GetTriangles() );
    mSurface->GenerateNormals();
   
	mTriMesh = TriMesh::create();
	for( const auto& vertex : loader.GetVertices() ) {
		mTriMesh->appendPosition( ci::vec3( vertex.x, vertex.y, vertex.z ) );
	}
	for( const auto& triangle : loader.GetTriangles() ) {
		mTriMesh->appendTriangle( triangle.index[0], triangle.index[1], triangle.index[2] );
	}
    
    timer.stop();
    CI_LOG_I("Finished loading Surface Mesh for : " << filename << " in " << timer.getSeconds() << " seconds " );
    return mSurface;
}

TetraTopologyRef TetraMesh::generateTetrasFromSurface( const TriangleTopologyRef& triMesh, const double cellSize, const double facetAngle, const double facetSize, const double facetDistance, const double cellRadiusEdgeRatio  )
{
    if( !triMesh ) return nullptr;
    
    ci::Timer timer;
    timer.start();
    const auto& tris = triMesh->GetTriangles();
    const auto& verts = triMesh->GetVertices();
    
    CGALTetrahedralizeRef cth = std::make_shared<CGALTetrahedralize>();
    
    try {
        cth->GenerateFromSurface( tris, verts, cellSize, facetAngle, facetSize, facetDistance, cellRadiusEdgeRatio );
    }
    
    catch( std::exception e ) {
        CI_LOG_E(" Failed to generate tetrahedral mesh. Most probably a CGAL precondition is violated..");
        if( cth ) {
            cth.reset();
        }
        return nullptr;
    }
    
    timer.stop();
    
    mTopology = std::make_shared<TetraTools::TetrahedronTopology>();
    mTopology->Init( cth->GetTetraVertices(), cth->GetTetras(), false );
    CI_LOG_I( "Generated tetrahedral mesh in : " << timer.getSeconds() << " with " << cth->GetTetras().size() << " tetras and " << cth->GetTetraVertices().size() << " vertices " );
    return mTopology;
}

} // namespace tetra
