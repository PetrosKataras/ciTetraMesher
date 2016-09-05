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

using namespace ci::app;

TetraMesh::TetraMesh( const fs::path& path,
                        const double cellSize,
                        const double facetAngle,
                        const double facetSize,
                        const double facetDistance,
                        const double cellRadiusEdgeRatio )
{
    generateTetraBatches( generateTetrasFromSurface( loadSurface( path.string() ), cellSize, facetAngle, facetSize, facetDistance, cellRadiusEdgeRatio ) );
}

void TetraMesh::draw()
{
    for( const auto& batch : mTetraBatches ) {
        if( batch ) 
            batch->draw();
    }
}

void TetraMesh::multiDrawIndirect()
{
    if( !mTopology || mTopology->GetTetrahedra().size() == 0 ) return;

    gl::ScopedGlslProg ScopedGlslProg( mTetraMDI->getGlslProg() );
    auto vao = mTetraMDI->getVao();
    gl::ScopedVao ScopedVao( vao );
    gl::setDefaultShaderVars();
    glMultiDrawArraysIndirect( GL_TRIANGLES, NULL, mTopology->GetTetrahedra().size(), 0 );
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



void TetraMesh::generateTetraBatches( const TetraTopologyRef& tetraTopology )
{
    if( !tetraTopology ) return;
    
    mVertices.clear();
    mNormals.clear();
    mCentroids.clear();

    const auto& tetrahedra = tetraTopology->GetTetrahedra();
    const auto& verts = tetraTopology->GetVertices();
    
    for( const auto& tetra : tetrahedra ) {
        
        std::vector<uint32_t> indices;
        std::vector<vec3> vertices;
        std::vector<vec3> normals;
    
        Vec3f c = tetra.Centroid( verts[tetra.index[0]], verts[tetra.index[1]], verts[tetra.index[2]], verts[tetra.index[3]] );
        
        vec3 centroid = vec3( c.x, c.y, c.z );
        mCentroids.push_back( centroid );
        //CI_LOG_I( "Centroid : " << centroid );
        vec3 v0 = vec3( verts[tetra.index[0]].x, verts[tetra.index[0]].y, verts[tetra.index[0]].z );
        vec3 v1 = vec3( verts[tetra.index[1]].x, verts[tetra.index[1]].y, verts[tetra.index[1]].z );
        vec3 v2 = vec3( verts[tetra.index[2]].x, verts[tetra.index[2]].y, verts[tetra.index[2]].z );
        vec3 v3 = vec3( verts[tetra.index[3]].x, verts[tetra.index[3]].y, verts[tetra.index[3]].z );
        
        vec3 vc0 = ( v0 - centroid );// * .90f;
        vec3 vc1 = ( v1 - centroid );// * .90f;
        vec3 vc2 = ( v2 - centroid );// * .90f;
        vec3 vc3 = ( v3 - centroid );// * .90f;
        
        vertices.push_back( centroid + vc0 );
        vertices.push_back( centroid + vc3 );
        vertices.push_back( centroid + vc2 );
        
        vertices.push_back( centroid + vc3 );
        vertices.push_back( centroid + vc1 );
        vertices.push_back( centroid + vc2 );
        
        vertices.push_back( centroid + vc1 );
        vertices.push_back( centroid + vc0 );
        vertices.push_back( centroid + vc2 );
        
        vertices.push_back( centroid + vc0 );
        vertices.push_back( centroid + vc1 );
        vertices.push_back( centroid + vc3 );

        CGALUtils::calculateAddTetraNormals( vertices, normals );
    
        std::vector<gl::VboMesh::Layout> bufferLayout {
            gl::VboMesh::Layout().usage( GL_STATIC_DRAW ).attrib( geom::Attrib::POSITION, 3 ),
            gl::VboMesh::Layout().usage( GL_STATIC_DRAW ).attrib( geom::Attrib::NORMAL, 3 )
        };
        //gl::VboMeshRef vboMesh = gl::VboMesh::create( vertices.size(), GL_TRIANGLES, bufferLayout, 0, GL_UNSIGNED_INT, 0 );
        //vboMesh->bufferAttrib( geom::POSITION, vertices.size() * sizeof( vec3 ), vertices.data() );
        //vboMesh->bufferAttrib( geom::NORMAL, normals.size() * sizeof( vec3 ), normals.data() );
    
        //mTetraBatches.push_back( gl::Batch::create( vboMesh, gl::getStockShader( gl::ShaderDef().lambert() ) ) );

        for( const auto& vertex : vertices ) {
            mVertices.push_back( vertex );
        }
        for( const auto& norm : normals ) {
            mNormals.push_back( norm );
        }
    }

    auto centroidsVbo = gl::Vbo::create( GL_ARRAY_BUFFER, mCentroids.size() * sizeof( vec3 ), mCentroids.data(), GL_STATIC_DRAW );
    geom::BufferLayout instanceCentroidsLayout;
    instanceCentroidsLayout.append( geom::Attrib::CUSTOM_0, 3, 0, 0, 1 );

    mModelMatrices.clear();
    mModelMatrices.resize( tetrahedra.size() );
    mModelMatricesVbo = gl::Vbo::create( GL_ARRAY_BUFFER, mModelMatrices.size() * sizeof( mat4 ), mModelMatrices.data(), GL_DYNAMIC_DRAW );
    geom::BufferLayout instanceModelMatricesLayout;
    instanceModelMatricesLayout.append( geom::Attrib::CUSTOM_1, 16, sizeof(mat4), 0, 1 );
    //centroidsVbo->bufferData( GL_ARRAY_BUFFER, mCentroids.data(), GL_STATIC_DRAW );

    mIndirectBuffer = gl::Vbo::create( GL_DRAW_INDIRECT_BUFFER );
    mIndirectBuffer->bufferData( tetrahedra.size() * sizeof( DrawArraysIndirectCommand ), NULL, GL_STATIC_DRAW );
    DrawArraysIndirectCommand * drawCmd = ( DrawArraysIndirectCommand* )mIndirectBuffer->mapBufferRange( 0, tetrahedra.size() * sizeof( DrawArraysIndirectCommand ), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT );
    for( auto i = 0; i < tetrahedra.size(); ++i ) {
        drawCmd[i].vertexCount = 12;
        drawCmd[i].instanceCount = 1;
        drawCmd[i].firstVertex = i*12;
        drawCmd[i].baseInstance = i;
    }
    mIndirectBuffer->unmap();

    std::vector<gl::VboMesh::Layout> vertLayout {
        gl::VboMesh::Layout().usage( GL_STATIC_DRAW ).attrib( geom::Attrib::POSITION, 3 ),
        gl::VboMesh::Layout().usage( GL_STATIC_DRAW ).attrib( geom::Attrib::NORMAL, 3 )
    };
    auto indirectVboMesh = gl::VboMesh::create( mVertices.size(), GL_TRIANGLES, vertLayout );
    indirectVboMesh->bufferAttrib( geom::Attrib::POSITION, mVertices );
    indirectVboMesh->bufferAttrib( geom::Attrib::NORMAL, mNormals );

    indirectVboMesh->appendVbo( instanceCentroidsLayout, centroidsVbo );
    indirectVboMesh->appendVbo( instanceModelMatricesLayout, mModelMatricesVbo );
    //mGlslMDI = gl::getStockShader( gl::ShaderDef().lambert().color() );
    mGlslMDI = gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "InstancedModelMatrix.vert" )).geometry( loadAsset("InstancedModelMatrix.geom") ).fragment( loadAsset( "InstancedModelMatrix.frag" ) ) );
    mTetraMDI = gl::Batch::create( indirectVboMesh, mGlslMDI, { { geom::Attrib::CUSTOM_0, "centroidPosition" }, { geom::Attrib::CUSTOM_1, "aModelMatrix" } } );
}

void TetraMesh::update()
{

}

} // namespace tetra
