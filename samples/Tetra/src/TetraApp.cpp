#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Log.h"
#include "cinder/CameraUi.h"
#include "cinder/params/Params.h"

#include "TetraMesh.h"
#include "DeferredRenderer.h"

using namespace ci;
using namespace ci::app;
using namespace std;

using namespace tetra;

using ParamGroup = std::vector<params::InterfaceGl::OptionsBase>; 

class TetraApp : public App {
  public:
    void setup() override;
    void mouseDown( MouseEvent event ) override;
    void mouseDrag( MouseEvent event ) override;
    void update() override;
    void resize() override;
    void draw() override;
    void keyDown( KeyEvent event ) override;

  private:
    void createParams();
    void toggleDebug();
    void updateCutPlane();
    void updateBoundingSphere();    
    void updateTetraScale();
    void generateTetraBatch( const TetraTopologyRef& tetraTopology );
    struct DrawArraysIndirectCommand{                                                     
        GLuint vertexCount;                                                               
        GLuint instanceCount;                                                             
        GLuint firstVertex;                                                               
        GLuint baseInstance;                                                              
    };            
  private:
    params::InterfaceGlRef      mParams;
    CameraPersp                 mCamera;
    CameraUi                    mCamUi;
    TetraMeshRef                mMesh;
    DeferredRendererRef         mDeferredRenderer;

    gl::BatchRef                mCutPlane;
    gl::BatchRef                mBoundingSphere;
    mat4                        mCutPlaneTransform;
    float                       mCutPlaneX, mCutPlaneY, mCutPlaneZ = 0.0f;
    float                       mCutPlaneDistance = 0.0f;
    float                       mTetraScaleFactor = 1.0f;
    bool                        mDrawCutPlane = false;
    float                       mBSphereRadius = 10.0f;
    float                       mBSphereCenterX = 0.0f;
    float                       mBSphereCenterY = 0.0f;
    float                       mBSphereCenterZ = 0.0f;
    bool                        mDrawBSphere = true;
    gl::BatchRef                mTetraMdiBatch;
    gl::GlslProgRef             mTetraMdiGlsl;
    std::vector<vec3>           mVertices;
    std::vector<vec3>           mNormals;
    std::vector<vec3>           mCentroids;
    std::vector<mat4>           mModelMatrices;
    gl::VboRef                  mIndirectBuffer;
    gl::VboRef                  mModelMatricesVbo;
    bool                        mEnableDebug = true;
};

void TetraApp::generateTetraBatch( const TetraTopologyRef& tetraTopology )
{
    if( ! tetraTopology ) return;

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

        vertices.push_back( v0 );
        vertices.push_back( v3 );
        vertices.push_back( v2 );

        vertices.push_back( v3 );
        vertices.push_back( v1 );
        vertices.push_back( v2 );

        vertices.push_back( v1 );
        vertices.push_back( v0 );
        vertices.push_back( v2 );

        vertices.push_back( v0 );
        vertices.push_back( v1 );
        vertices.push_back( v3 );

        CGALUtils::calculateAddTetraNormals( vertices, normals );

        std::vector<gl::VboMesh::Layout> bufferLayout {
            gl::VboMesh::Layout().usage( GL_STATIC_DRAW ).attrib( geom::Attrib::POSITION, 3 ),
            gl::VboMesh::Layout().usage( GL_STATIC_DRAW ).attrib( geom::Attrib::NORMAL, 3 )
        };

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
    DrawArraysIndirectCommand * drawCmd = ( DrawArraysIndirectCommand* )mIndirectBuffer->mapBufferRange( 0, tetrahedra.size() * sizeof( DrawArraysIndirectCommand ),         GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT );
    CI_CHECK_GL();
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
    mTetraMdiGlsl = gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "tetra.vert" ) ).geometry(
    loadAsset("tetra.geom") ).fragment( loadAsset( "tetra.frag" ) ).define( "ENABLE_GBUFFER_PASS" ) );
    mTetraMdiBatch = gl::Batch::create( indirectVboMesh, mTetraMdiGlsl, { { geom::Attrib::CUSTOM_0, "centroidPosition" }, { geom::Attrib::CUSTOM_1, "aModelMatrix" } } );
}

void TetraApp::resize()
{
    mCamera.setAspectRatio( getWindowAspectRatio() );
}

void TetraApp::createParams()
{
    mParams = params::InterfaceGl::create( getWindow(), "Tetra Control", toPixels( ivec2( 300, 400 ) ) );
    mParams->addParam( "CutPlaneX", &mCutPlaneX ).step( .01f ).min( -1.0f).max(1.0f).updateFn( std::bind( &TetraApp::updateCutPlane, this  ) );
    mParams->addParam( "CutPlaneY", &mCutPlaneY ).step( .01f ).min( -1.0f).max(1.0f).updateFn( std::bind( &TetraApp::updateCutPlane, this  ) );
    mParams->addParam( "CutPlaneZ", &mCutPlaneZ ).step( .01f ).min( -1.0f).max(1.0f).updateFn( std::bind( &TetraApp::updateCutPlane, this  ) );
    mParams->addParam( "CutPlaneDistance", &mCutPlaneDistance ).step( 0.1f ).min( -40.0f).max(40.0f).updateFn( std::bind( &TetraApp::updateCutPlane, this  ) );
    mParams->addParam( "Draw Cut Plane", &mDrawCutPlane );
    mParams->addSeparator();
    mParams->addParam( "BSphereRadius", &mBSphereRadius ).step( 1.0f ).updateFn( std::bind( &TetraApp::updateBoundingSphere, this ) );
    mParams->addParam( "BSphereCenterX", &mBSphereCenterX ).step( 1.0f ).updateFn( std::bind( &TetraApp::updateBoundingSphere, this ) );
    mParams->addParam( "BSphereCenterY", &mBSphereCenterY ).step( 1.0f ).updateFn( std::bind( &TetraApp::updateBoundingSphere, this ) );
    mParams->addParam( "BSphereCenterZ", &mBSphereCenterZ ).step( 1.0f ).updateFn( std::bind( &TetraApp::updateBoundingSphere, this ) );
    mParams->addParam( "Draw Bounding Sphere", &mDrawBSphere );
    mParams->addSeparator();
    mParams->addParam( "Tetra Scale", &mTetraScaleFactor ).step( .01f ).min( .0f).max(1.0f).updateFn( std::bind(
    &TetraApp::updateTetraScale, this ) );
    mParams->addSeparator();
    mParams->addParam( "Toggle debug", &mEnableDebug ).updateFn( std::bind( &TetraApp::toggleDebug, this ) );
}

void TetraApp::updateTetraScale()
{
    if( ! mTetraMdiBatch ) return;

    mTetraMdiBatch->getGlslProg()->uniform( "tetraScaleFactor", mTetraScaleFactor );
}

void TetraApp::toggleDebug()
{
    if( mDeferredRenderer )
        mDeferredRenderer->enableDebugMode( mEnableDebug );
}

void TetraApp::updateCutPlane()
{
    auto cutPlaneVboMesh = gl::VboMesh::create( geom::Plane().axes( vec3( 0.0f, 1.0f, 0.0f ), vec3( 1.0f, 0.0f, 0.0f ) ).subdivisions( vec2( 5 ) ).size( vec2( 30 ) ).normal( vec3( mCutPlaneX, mCutPlaneY, mCutPlaneZ ) ) );
    auto stockColorGlsl = gl::context()->getStockShader( gl::ShaderDef().color() );
    mCutPlane = gl::Batch::create( cutPlaneVboMesh, stockColorGlsl );

}

void TetraApp::updateBoundingSphere()
{
    auto stockColorGlsl = gl::context()->getStockShader( gl::ShaderDef().color() );
    mBoundingSphere = gl::Batch::create( geom::Sphere().radius( mBSphereRadius ).center( vec3( mBSphereCenterX, mBSphereCenterY, mBSphereCenterZ ) ), stockColorGlsl );
}

void TetraApp::keyDown( KeyEvent event )
{
    if ( event.getCode() == KeyEvent::KEY_ESCAPE ) quit();
}


void TetraApp::setup()
{
    updateCutPlane();

    // cellSize, facetAngle, facetSize, facetDistance, cellEdgeRatio
    mMesh = TetraMesh::create( getAssetPath("8lbs.off"), 10.0, 20, 1.4, 0.8, 3 );
    generateTetraBatch( mMesh->getTopology() );
    auto boundingSphere = Sphere::calculateBoundingSphere( mMesh->getTriMesh()->getPositions<3>(), mMesh->getTriMesh()->getNumVertices() );
    mCamera = mCamera.calcFraming( boundingSphere );
    mCamUi = CameraUi( &mCamera );
    
    mDeferredRenderer = DeferredRenderer::create();

    createParams();

    gl::enableDepthRead();
    gl::enableDepthWrite();
}

void TetraApp::mouseDown( MouseEvent event )
{
    mCamUi.mouseDown( event );
}

void TetraApp::mouseDrag( MouseEvent event )
{
    mCamUi.mouseDrag( event );
}

void TetraApp::update()
{
    getWindow()->setTitle( "FPS : " +std::to_string( getAverageFps() ) );
    mCutPlaneTransform = mat4();
    const vec3 fromVec = vec3( 0, 0, 1 );
    vec3 toVec = normalize( vec3( mCutPlaneX, mCutPlaneY, mCutPlaneZ ) ); 
    auto q = glm::quat( fromVec, toVec );
    auto m = glm::toMat4( q );
    mCutPlaneTransform *= glm::translate( vec3( 0, 0, mCutPlaneDistance ) );
    mCutPlaneTransform *= m;
    
    if( mDeferredRenderer )
        mDeferredRenderer->update();
}

void TetraApp::draw()
{
    gl::clear( Color( .5, .5, .5 ) );
    if( mMesh ) {
        mTetraMdiBatch->getGlslProg()->uniform("boundingSphere", vec4( mBSphereCenterX, mBSphereCenterY, mBSphereCenterZ, mBSphereRadius ) );
        if( mDeferredRenderer && mTetraMdiBatch && mIndirectBuffer )
            mDeferredRenderer->render( mTetraMdiBatch, mIndirectBuffer, mMesh->getTopology()->GetTetrahedra().size(),
            mCamera );
    }
    {
        gl::ScopedMatrices scopedMatrices;
        gl::setMatrices( mCamera );
        if( mCutPlane && mDrawCutPlane ) {
            gl::enableWireframe();
            gl::ScopedColor colorScope( Color( CM_RGB, .0f, .0f, .0f ) );
            gl::ScopedModelMatrix model;
            gl::multModelMatrix( mCutPlaneTransform );
            mCutPlane->draw();
            gl::disableWireframe();
        }

        if( mBoundingSphere && mDrawBSphere ) {
            gl::enableWireframe();
            gl::ScopedColor colorScope( Color( CM_RGB, .0f, .0f, .0f ) );
            mBoundingSphere->draw();
            gl::disableWireframe();
        }
    }

    gl::drawCoordinateFrame(1);

    if( mParams ) {
        gl::ScopedDepth depthScope(false);
        mParams->draw();
    }
}

CINDER_APP( TetraApp, RendererGl(RendererGl::Options().msaa(16)), [&](App::Settings *settings){
    settings->setWindowSize( 1300, 800 );
    settings->setMultiTouchEnabled( false );
}  )
