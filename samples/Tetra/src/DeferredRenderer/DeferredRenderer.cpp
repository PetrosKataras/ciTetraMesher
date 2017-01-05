#include "DeferredRenderer.h"
#include "cinder/Log.h"
#include "cinder/Utilities.h"

using namespace ci;
using namespace std;
namespace tetra {

DeferredRenderer::DeferredRenderer()
{
    gl::disableAlphaBlending();
    // Set up lights
    mLights.push_back( Light().colorDiffuse( ColorAf( 0.35f, .2f, 0.22f, 1.0f ) )
                      .intensity( 0.80f ).position( vec3( 0.0f, 0.0f, 0.0f ) )
                      .radius( 8.1f ).volume( 85.0f ) );
    for ( size_t i = 0; i < 8; ++i ) {
        const float t = ( (float)i / 8.0f ) * 0.3f;
        mLights.push_back( Light().colorDiffuse( ColorAf( 0.70f + t * 0.5f, 0.2f - t, 0.1f + t * t, 1.0f ) )
                          .intensity( 0.75f ).radius( 7.05f ).volume( 65.0f ) );
    }

    // Color texture format
    const gl::Texture2d::Format colorTextureFormat = gl::Texture2d::Format()
    .internalFormat( GL_RGBA8 )
    .magFilter( GL_NEAREST )
    .minFilter( GL_NEAREST )
    .wrap( GL_CLAMP_TO_EDGE );
    
    // Data texture format
    const gl::Texture2d::Format dataTextureFormat = gl::Texture2d::Format()
    .internalFormat( GL_RGBA16F )
    .magFilter( GL_NEAREST )
    .minFilter( GL_NEAREST )
    .wrap( GL_CLAMP_TO_EDGE );
    
    // Create FBO for the the geometry buffer (G-buffer). This G-buffer uses
    // three color attachments to store position, normal, emissive (light), and
    // color (albedo) data.
    const ivec2 winSize = getWindowSize();
    const int32_t h     = winSize.y;
    const int32_t w     = winSize.x;
    try {
        gl::Fbo::Format fboFormat;
        mTextureFboGBuffer[ 0 ] = gl::Texture2d::create( w, h, colorTextureFormat );
        mTextureFboGBuffer[ 1 ] = gl::Texture2d::create( w, h, dataTextureFormat );
        mTextureFboGBuffer[ 2 ] = gl::Texture2d::create( w, h, dataTextureFormat );
        for ( size_t i = 0; i < 3; ++i ) {
            fboFormat.attachment( GL_COLOR_ATTACHMENT0 + (GLenum)i, mTextureFboGBuffer[ i ] );
        }
        fboFormat.depthTexture();
        mFboGBuffer = gl::Fbo::create( w, h, fboFormat );
        const gl::ScopedFramebuffer scopedFramebuffer( mFboGBuffer );
        const gl::ScopedViewport scopedViewport( ivec2( 0 ), mFboGBuffer->getSize() );
        gl::clear();
    }
    catch( const std::exception& e ) {
        console() << "mFboGBuffer failed: " << e.what() << std::endl;
    }
    
    // Create FBO for the light buffer (L-buffer). The L-buffer reads the
    // G-buffer textures to render the scene inside light volumes.
    try {
        mFboLBuffer = gl::Fbo::create( w, h, gl::Fbo::Format().colorTexture() );
        const gl::ScopedFramebuffer scopedFramebuffer( mFboLBuffer );
        const gl::ScopedViewport scopedViewport( ivec2( 0 ), mFboLBuffer->getSize() );
        gl::clear();
    }
    catch( const std::exception& e ) {
        console() << "mFboLBuffer failed: " << e.what() << std::endl;
    }
    
    createBatches();
}

void DeferredRenderer::createBatches()
{
    // Shortcut for shader loading and error handling
    auto loadGlslProg = [ & ]( const gl::GlslProg::Format& format ) -> gl::GlslProgRef
    {
        const std::string names = format.getVertexPath().string() + " + " + format.getFragmentPath().string();
        gl::GlslProgRef glslProg;
        try {
            glslProg = gl::GlslProg::create( format );
        } catch ( const gl::Exception& ex ) {
            CI_LOG_EXCEPTION( names, ex );
        }
        return glslProg;
    };
    
    // Load shader files from disk
    const DataSourceRef fragDebug       = loadAsset( "debug.frag" );
    const DataSourceRef fragFxaa        = loadAsset( "fxaa.frag" );
    const DataSourceRef fragGBuffer     = loadAsset( "gbuffer.frag" );
    const DataSourceRef fragLBuffer     = loadAsset( "lbuffer.frag" );
    const DataSourceRef vertGBuffer     = loadAsset( "gbuffer.vert" );
    const DataSourceRef vertPassThrough = loadAsset( "pass_through.vert" );

    int32_t version = 330;
    const gl::GlslProgRef debug         = loadGlslProg( gl::GlslProg::Format().version( version )
                                                       .vertex( vertPassThrough ).fragment( fragDebug ) );
    const gl::GlslProgRef fxaa          = loadGlslProg( gl::GlslProg::Format().version( version )
                                                       .vertex( vertPassThrough ).fragment( fragFxaa ) );
    const gl::GlslProgRef gBuffer       = loadGlslProg( gl::GlslProg::Format().version( version )
                                                       .vertex( vertGBuffer ).fragment( fragGBuffer ) );
    const gl::GlslProgRef lBuffer       = loadGlslProg( gl::GlslProg::Format().version( version )
                                                       .vertex( vertPassThrough ).fragment( fragLBuffer ) );
    const gl::GlslProgRef stockColor    = gl::context()->getStockShader( gl::ShaderDef().color() );
    const gl::GlslProgRef stockTexture  = gl::context()->getStockShader( gl::ShaderDef().texture( GL_TEXTURE_2D ) );

    const gl::VboMeshRef cube   = gl::VboMesh::create( geom::Cube().size( vec3( 2.0f ) ) );
    const gl::VboMeshRef plane  = gl::VboMesh::create( geom::Plane()
                                                      .axes( vec3( 0.0f, 1.0f, 0.0f ), vec3( 0.0f, 0.0f, 1.0f ) )
                                                      .normal( vec3( 0.0f, 1.0f, 0.0f ) )
                                                      .size( vec2( 100.0f ) ) );
    const gl::VboMeshRef rect   = gl::VboMesh::create( geom::Rect() );
    const gl::VboMeshRef sphere = gl::VboMesh::create( geom::Sphere().subdivisions( 64 ) );

    mBatchDebugRect         = gl::Batch::create( rect,      debug );
    mBatchFxaaRect          = gl::Batch::create( rect,      fxaa );
    mBatchGBufferPlane      = gl::Batch::create( plane,     gBuffer );
    mBatchLBufferCube       = gl::Batch::create( cube,      lBuffer );
    mBatchGBufferSphere     = gl::Batch::create( sphere,    gBuffer );
   
    mBatchDebugRect->getGlslProg()->uniform(    "uSamplerAlbedo",           0 );
    mBatchDebugRect->getGlslProg()->uniform(    "uSamplerNormalEmissive",   1 );
    mBatchDebugRect->getGlslProg()->uniform(    "uSamplerPosition",         2 );
    mBatchFxaaRect->getGlslProg()->uniform(     "uSampler",                 0 );
    mBatchLBufferCube->getGlslProg()->uniform(  "uSamplerAlbedo",           0 );
    mBatchLBufferCube->getGlslProg()->uniform(  "uSamplerNormalEmissive",   1 );
    mBatchLBufferCube->getGlslProg()->uniform(  "uSamplerPosition",         2 );
    mBatchLBufferCube->getGlslProg()->uniform(  "uSamplerShadowMap",        3 );
}

void DeferredRenderer::update()
{
    // Update light positions
    if ( !mLights.empty() ) {
        const float e           = (float)getElapsedSeconds();
        const size_t numLights = mLights.size() - 1;
        float t                 = e;
        const float d           = ( (float)M_PI * 2.0f ) / (float)numLights;
        const float r           = 63.5f;
        for ( size_t i = 0; i < numLights; ++i, t += d ) {
            const float ground  = 0.1f;
            const float x       = glm::cos( t );
            const float z       = glm::sin( t );
            vec3 p              = vec3( x, 0.0f, z ) * r;
            p.y                 = ground + 1.5f;
            mLights.at( i + 1 ).setPosition( p );
        }
        
        t                   = e * 0.333f;
        mLights.front().setPosition( vec3( glm::sin( t ), 30.0f, glm::cos( t ) ) );
    }
}

void DeferredRenderer::render( const gl::BatchRef& gBatch, const gl::VboRef& indirectBuffer, const uint16_t& numTetras, const CameraPersp& camera )
{
    vector<mat4> spheres;
    {
        mat4 m( 1.0f );
        m = glm::translate( m, vec3( 0.0f, 4.0f, 0.0f ) );
        spheres.push_back( m );
    }
    const float e       = (float)getElapsedSeconds();
    float t             = e * 0.165f;
    const float d = ( (float)M_PI * 2.0f ) / 4.0f;
    const float r = 4.5f;
    for ( size_t i = 0; i < 4; ++i, t += d ) {
        const float x   = glm::cos( t );
        const float z   = glm::sin( t );
        vec3 p          = vec3( x, 0.0f, z ) * r;
        p.y             = 0.5f;
        
        mat4 m( 1.0f );
        m = glm::translate( m, p );
        m = glm::rotate( m, e, vec3( 1.0f ) );
        m = glm::scale( m, vec3( 0.5f ) );
        
        spheres.push_back( m );
    }
    
    //////////////////////////////////////////////////////////////////////////////////////////////
    // G-BUFFER
    
    {
        // Bind the G-buffer FBO and draw to all attachments
        const gl::ScopedFramebuffer scopedFrameBuffer( mFboGBuffer );
        {
            const static GLenum buffers[] = {
                GL_COLOR_ATTACHMENT0,
                GL_COLOR_ATTACHMENT1,
                GL_COLOR_ATTACHMENT2
            };
            gl::drawBuffers( 3, buffers );
        }
        const gl::ScopedViewport scopedViewport( ivec2( 0 ), mFboGBuffer->getSize() );
        const gl::ScopedMatrices scopedMatrices;
        gl::enableDepthRead();
        gl::enableDepthWrite();
        gl::clear();
        gl::setMatrices( camera );
        
        if( mDrawFloor ) {
            mBatchGBufferPlane->getGlslProg()->uniform( "uEmissive", 0.0f );
            mBatchGBufferPlane->draw();
        }

        const gl::ScopedFaceCulling scopedFaceCulling( true, GL_BACK );
        if( gBatch ) {
            gl::ScopedGlslProg scopedGlslProg( gBatch->getGlslProg() );
            gBatch->getGlslProg()->uniform( "uEmissive", 0.0f );
            auto vao = gBatch->getVao();
            gl::ScopedVao scopedVao( vao );
            gl::setDefaultShaderVars();
            gl::ScopedBuffer scopedBuffer( indirectBuffer );
            glMultiDrawArraysIndirect( GL_TRIANGLES, NULL, numTetras, 0 );
        }
        
        if( mDrawLights ) {
            // Draw light sources
            mBatchGBufferSphere->getGlslProg()->uniform( "uEmissive", 1.0f );
            for ( const Light& light : mLights ) {
                const gl::ScopedModelMatrix scopedModelMatrix;
                const gl::ScopedColor scopedColor( light.getColorDiffuse() );
                gl::translate( light.getPosition() );
                gl::scale( vec3( light.getRadius() ) );
                mBatchGBufferSphere->draw();
            }
        }
    }
    
    //////////////////////////////////////////////////////////////////////////////////////////////
    // L-BUFFER
    
    {
        const gl::ScopedFramebuffer scopedFrameBuffer( mFboLBuffer );
        const gl::ScopedViewport scopedViewport( ivec2( 0 ), mFboLBuffer->getSize() );
        gl::clear();
        const gl::ScopedMatrices scopedMatrices;
        const gl::ScopedBlendAdditive scopedAdditiveBlend;
        const gl::ScopedFaceCulling scopedFaceCulling( true, GL_FRONT );
        gl::setMatrices( camera );
        gl::enableDepthRead();
        gl::disableDepthWrite();
        
        // Bind G-buffer textures and shadow map
        const gl::ScopedTextureBind scopedTextureBind0( mTextureFboGBuffer[ 0 ],            0 );
        const gl::ScopedTextureBind scopedTextureBind1( mTextureFboGBuffer[ 1 ],            1 );
        const gl::ScopedTextureBind scopedTextureBind2( mTextureFboGBuffer[ 2 ],            2 );
        
        // Draw light volumes
        mBatchLBufferCube->getGlslProg()->uniform( "uShadowEnabled",            false );
        mBatchLBufferCube->getGlslProg()->uniform( "uShadowMatrix",             mat4() );
        mBatchLBufferCube->getGlslProg()->uniform( "uViewMatrixInverse",        camera.getInverseViewMatrix() );
        
        for ( const Light& light : mLights ) {
            mBatchLBufferCube->getGlslProg()->uniform( "uLightColorAmbient",    light.getColorAmbient() );
            mBatchLBufferCube->getGlslProg()->uniform( "uLightColorDiffuse",    light.getColorDiffuse() );
            mBatchLBufferCube->getGlslProg()->uniform( "uLightColorSpecular",   light.getColorSpecular() );
            mBatchLBufferCube->getGlslProg()->uniform( "uLightPosition",
                                                      vec3( ( camera.getViewMatrix() * vec4( light.getPosition(), 1.0 ) ) ) );
            mBatchLBufferCube->getGlslProg()->uniform( "uLightIntensity",       light.getIntensity() );
            mBatchLBufferCube->getGlslProg()->uniform( "uLightRadius",          light.getVolume() );
            mBatchLBufferCube->getGlslProg()->uniform( "uWindowSize",           vec2( getWindowSize() ) );
            
            gl::ScopedModelMatrix scopedModelMatrix;
            gl::translate( light.getPosition() );
            gl::scale( vec3( light.getVolume() ) );
            mBatchLBufferCube->draw();
        }
    }
    
    //////////////////////////////////////////////////////////////////////////////////////////////
    // FINAL RENDER
    
    // Set up window for screen render
    const gl::ScopedViewport scopedViewport( ivec2( 0 ), toPixels( getWindowSize() ) );
    const gl::ScopedMatrices scopedMatrices;
    gl::disableDepthRead();
    gl::disableDepthWrite();
    gl::clear();
    
    if ( mDebugMode ) {
        
        // Draw G-buffer
        gl::clear( Colorf::gray( 0.4f ) );
        gl::setMatricesWindow( getWindowSize() );
        const gl::ScopedTextureBind scopedTextureBind0( mTextureFboGBuffer[ 0 ], 0 );
        const gl::ScopedTextureBind scopedTextureBind1( mTextureFboGBuffer[ 1 ], 1 );
        const gl::ScopedTextureBind scopedTextureBind2( mTextureFboGBuffer[ 2 ], 2 );
        
        // Albedo   | Normals
        // --------------------
        // Position | Emissive
        vec2 sz = getWindowCenter();
        for ( int32_t i = 0; i < 4; ++i ) {
            const vec2 pos( ( i % 2 ) * sz.x, glm::floor( (float)i * 0.5f ) * sz.y );
            gl::ScopedModelMatrix scopedModelMatrix;
            gl::translate( pos + sz * 0.5f );
            gl::scale( sz );
            mBatchDebugRect->getGlslProg()->uniform( "uMode", i );
            mBatchDebugRect->draw();
        }
    } else {
        gl::translate( getWindowCenter() );
        gl::scale( getWindowSize() );
        
        const gl::ScopedTextureBind scopedTextureBind( mFboLBuffer->getColorTexture(), 0 );
            
        // Perform FXAA
        mBatchFxaaRect->getGlslProg()->uniform( "uPixel", vec2( 1.0f ) / vec2( toPixels( getWindowSize() ) ) );
        mBatchFxaaRect->draw();
    }
    
}

void DeferredRenderer::enableDebugMode( bool enable )
{
    mDebugMode = enable;
}
} // namespace tetra

