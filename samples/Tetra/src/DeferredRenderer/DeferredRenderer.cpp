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
    .internalFormat( GL_RGB10_A2 )
    .magFilter( GL_NEAREST )
    .minFilter( GL_NEAREST )
    .wrap( GL_CLAMP_TO_EDGE )
    .dataType( GL_FLOAT );
    
    // Data texture format
    const gl::Texture2d::Format dataTextureFormat = gl::Texture2d::Format()
    .internalFormat( GL_RGBA16F )
    .magFilter( GL_NEAREST )
    .minFilter( GL_NEAREST )
    .wrap( GL_CLAMP_TO_EDGE );

    // Texture format for depth buffers
    gl::Texture2d::Format depthTextureFormat = gl::Texture2d::Format()
    .internalFormat( GL_DEPTH_COMPONENT32F )
    .magFilter( GL_LINEAR )
    .minFilter( GL_LINEAR )
    .wrap( GL_CLAMP_TO_EDGE )
    .dataType( GL_FLOAT );
    
    // Create FBO for the the geometry buffer (G-buffer). This G-buffer uses
    // three color attachments to store position, normal, emissive (light), and
    // color (albedo) data.
    const ivec2 winSize = getWindowSize();
    const int32_t h     = winSize.y;
    const int32_t w     = winSize.x;
    mOffset = vec2( 0.0f );// vec2( w, h ) * vec2( .1f );
    try {
        const ivec2 sz = ivec2(  vec2( w, h ) + mOffset * 2.0f );
        gl::Fbo::Format fboFormat;
        mTextureFboGBuffer[ 0 ] = gl::Texture2d::create( sz.x, sz.y, colorTextureFormat );
        mTextureFboGBuffer[ 1 ] = gl::Texture2d::create( sz.x, sz.y, dataTextureFormat );
        mTextureFboGBuffer[ 2 ] = gl::Texture2d::create( sz.x, sz.y, dataTextureFormat );
        for ( size_t i = 0; i < 3; ++i ) {
            fboFormat.attachment( GL_COLOR_ATTACHMENT0 + (GLenum)i, mTextureFboGBuffer[ i ] );
        }
        fboFormat.depthTexture( depthTextureFormat );
        mFboGBuffer = gl::Fbo::create( sz.x, sz.y, fboFormat );
        const gl::ScopedFramebuffer scopedFramebuffer( mFboGBuffer );
        const gl::ScopedViewport scopedViewport( ivec2( 0 ), mFboGBuffer->getSize() );
        gl::clear();
    }
    catch( const std::exception& e ) {
        console() << "mFboGBuffer failed: " << e.what() << std::endl;
    }
    {
        ivec2 sz = mFboGBuffer->getSize() / 2; 
        gl::Fbo::Format fboFormat;
        fboFormat.disableDepth();
        for ( size_t i = 0; i < 2; ++i ) {
            mTextureFboAo[ i ] = gl::Texture2d::create( sz.x, sz.y, gl::Texture2d::Format()
                                                       .internalFormat( GL_RG32F )
                                                       .magFilter( GL_LINEAR )
                                                       .minFilter( GL_LINEAR )
                                                       .wrap( GL_CLAMP_TO_EDGE )
                                                       .dataType( GL_FLOAT ) ); 
            fboFormat.attachment( GL_COLOR_ATTACHMENT0 + (GLenum)i, mTextureFboAo[ i ] ); 
        }    
        mFboAo = gl::Fbo::create( sz.x, sz.y, fboFormat );
        {    
            const gl::ScopedFramebuffer scopedFramebuffer( mFboAo );
            const gl::ScopedViewport scopedViewport( ivec2( 0 ), mFboAo->getSize() );
            gl::clear();
        }    
             
        // Set up the SAO mip-map (clip-space Z) buffer
        {
            gl::Texture2d::Format cszTextureFormat = gl::Texture2d::Format()
                .internalFormat( GL_R32F )
                .mipmap()
                .magFilter( GL_NEAREST_MIPMAP_NEAREST )
                .minFilter( GL_NEAREST_MIPMAP_NEAREST )
                .wrap( GL_CLAMP_TO_EDGE )
                .dataType( GL_FLOAT );
            cszTextureFormat.setMaxMipmapLevel( 5 );
            mFboCsz = gl::Fbo::create( mFboGBuffer->getWidth(), mFboGBuffer->getHeight(),
                                      gl::Fbo::Format().disableDepth().colorTexture( cszTextureFormat ) ); 
            const gl::ScopedFramebuffer scopedFramebuffer( mFboCsz );
            const gl::ScopedViewport scopedViewport( ivec2( 0 ), mFboCsz->getSize() );
            gl::clear();
        }    

    }
    // Set up the ping pong frame buffer. We'll use this FBO to render
    // the scene and perform post-processing passes.
    {
        gl::Fbo::Format fboFormat;
        fboFormat.disableDepth();
        for ( size_t i = 0; i < 2; ++i ) {
            mTextureFboPingPong[ i ] = gl::Texture2d::create( w, h, colorTextureFormat );
            fboFormat.attachment( GL_COLOR_ATTACHMENT0 + (GLenum)i, mTextureFboPingPong[ i ] );
        }
        mFboPingPong = gl::Fbo::create( w, h, fboFormat );
        const gl::ScopedFramebuffer scopedFramebuffer( mFboPingPong );
        const gl::ScopedViewport scopedViewport( ivec2( 0 ), mFboPingPong->getSize() );
        gl::clear();
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
	const DataSourceRef fragAoComposite = loadAsset( "ao/composite.frag" );
	const DataSourceRef fragAoSaoAo     = loadAsset( "ao/ao.frag" );
	const DataSourceRef fragAoSaoBlur   = loadAsset( "ao/blur.frag" );
	const DataSourceRef fragAoSaoCsz	= loadAsset( "ao/csz.frag" );

    const DataSourceRef fragDebug       = loadAsset( "debug.frag" );
    const DataSourceRef fragFxaa        = loadAsset( "fxaa.frag" );
    const DataSourceRef fragGBuffer     = loadAsset( "gbuffer.frag" );
    const DataSourceRef fragLBuffer     = loadAsset( "lbuffer.frag" );
    const DataSourceRef vertGBuffer     = loadAsset( "gbuffer.vert" );
    const DataSourceRef vertPassThrough = loadAsset( "pass_through.vert" );

    int32_t version = 330;
	const gl::GlslProgRef aoComposite   = loadGlslProg( gl::GlslProg::Format().version( version )
                                                       .vertex( vertPassThrough ).fragment( fragAoComposite ) );
	const gl::GlslProgRef aoSaoAo       = loadGlslProg( gl::GlslProg::Format().version( version )
                                                       .vertex( vertPassThrough ).fragment( fragAoSaoAo ) );
	const gl::GlslProgRef aoSaoBlur		= loadGlslProg( gl::GlslProg::Format().version( version )
                                                        .vertex( vertPassThrough ).fragment( fragAoSaoBlur ) );
	const gl::GlslProgRef aoSaoCsz		= loadGlslProg( gl::GlslProg::Format().version( version )
                                                       .vertex( vertPassThrough ).fragment( fragAoSaoCsz ) );
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

	mBatchAoCompositeRect   = gl::Batch::create( rect,		aoComposite );
	mBatchSaoAoRect			= gl::Batch::create( rect,		aoSaoAo );
	mBatchSaoBlurRect		= gl::Batch::create( rect,		aoSaoBlur );
	mBatchSaoCszRect		= gl::Batch::create( rect,		aoSaoCsz );
    mBatchDebugRect         = gl::Batch::create( rect,      debug );
    mBatchFxaaRect          = gl::Batch::create( rect,      fxaa );
    mBatchGBufferPlane      = gl::Batch::create( plane,     gBuffer );
    mBatchLBufferCube       = gl::Batch::create( cube,      lBuffer );
    mBatchGBufferSphere     = gl::Batch::create( sphere,    gBuffer );
   
	const vec2 szGBuffer	= mFboGBuffer	? mFboGBuffer->getSize()	: toPixels( getWindowSize() );
	const vec2 szPingPong	= mFboPingPong	? mFboPingPong->getSize()	: toPixels( getWindowSize() );
	mBatchAoCompositeRect->getGlslProg()->uniform(		"uOffset",		mOffset );
	mBatchAoCompositeRect->getGlslProg()->uniform(		"uWindowSize",	szGBuffer );
	mBatchAoCompositeRect->getGlslProg()->uniform(		"uSampler",				0 );
	mBatchAoCompositeRect->getGlslProg()->uniform(		"uSamplerAo",			1 );
	mBatchSaoAoRect->getGlslProg()->uniform(			"uSampler",				0 );
	mBatchSaoBlurRect->getGlslProg()->uniform(			"uSampler",				0 );
	mBatchSaoCszRect->getGlslProg()->uniform(			"uSamplerDepth",		0 );
    mBatchDebugRect->getGlslProg()->uniform(    "uSamplerAlbedo",           0 );
    mBatchDebugRect->getGlslProg()->uniform(    "uSamplerNormalEmissive",   1 );
    mBatchDebugRect->getGlslProg()->uniform(    "uSamplerPosition",         2 );
    mBatchFxaaRect->getGlslProg()->uniform(     "uSampler",                 0 );
    mBatchLBufferCube->getGlslProg()->uniform(  "uSamplerAlbedo",           0 );
    mBatchLBufferCube->getGlslProg()->uniform(  "uSamplerNormalEmissive",   1 );
    mBatchLBufferCube->getGlslProg()->uniform(  "uSamplerPosition",         2 );
    mBatchLBufferCube->getGlslProg()->uniform(  "uSamplerShadowMap",        3 );
    mBatchLBufferCube->getGlslProg()->uniform(  "uOffset",        mOffset );
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
    const float f                   = camera.getFarClip();
    const float n                   = camera.getNearClip();
    const vec2 projectionParams     = vec2( f / ( f - n ), ( -f * n ) / ( f - n ) );
    const mat4 projMatrixInverse    = glm::inverse( camera.getProjectionMatrix() );

    
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
    size_t ping = 0;
    size_t pong = 1; 
    {
        const gl::ScopedFramebuffer scopedFrameBuffer( mFboPingPong );
        const gl::ScopedViewport scopedViewport( ivec2( 0 ), mFboPingPong->getSize() );
       {
            const static GLenum buffers[] = {
                GL_COLOR_ATTACHMENT0,
                GL_COLOR_ATTACHMENT1
            };
            gl::drawBuffers( 2, buffers );
            gl::clear();
        }

        gl::drawBuffer( GL_COLOR_ATTACHMENT0 + (GLenum)ping );

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
        ping = pong;
        pong = ( ping + 1 ) % 2;
    }
    // AO
    {
        // Convert depth to clip-space Z if we're performing SAO
        const gl::ScopedFramebuffer scopedFrameBuffer( mFboCsz );
        const gl::ScopedViewport scopedViewport( ivec2( 0 ), mFboCsz->getSize() );
        gl::drawBuffer( GL_COLOR_ATTACHMENT0 );
        gl::clear();
        gl::disableDepthWrite();
        const gl::ScopedMatrices scopedMatrices;
        gl::setMatricesWindow( mFboCsz->getSize() );
        gl::translate( mFboCsz->getSize() / 2 );
        gl::scale( mFboCsz->getSize() );
        gl::enableDepthRead();

        const gl::ScopedTextureBind scopedTextureBind( mFboGBuffer->getDepthTexture(), 0 );
        mBatchSaoCszRect->getGlslProg()->uniform( "uNear", n );
        mBatchSaoCszRect->draw();

        gl::disableDepthRead();
    }
    {
        // Clear AO buffer whether we use it or not
        const gl::ScopedFramebuffer scopedFrameBuffer( mFboAo );
        const gl::ScopedViewport scopedViewport( ivec2( 0 ), mFboAo->getSize() );
        gl::clear();
        // Draw next pass into AO buffer's first attachment
        const gl::ScopedMatrices scopedMatrices;
        gl::setMatricesWindow( mFboAo->getSize() );
        gl::enableDepthRead();
        gl::disableDepthWrite();
        gl::translate( mFboAo->getSize() / 2 );
        gl::scale( mFboAo->getSize() );
        const gl::ScopedBlendPremult scopedBlendPremult;
        // SAO (Scalable Ambient Obscurance)
        const gl::ScopedTextureBind scopedTextureBind( mFboCsz->getColorTexture(), 0 );
        const int32_t h = mFboPingPong->getHeight();
        const int32_t w = mFboPingPong->getWidth();
        const mat4& m   = camera.getProjectionMatrix();
        const vec4 p    = vec4( -2.0f / ( w * m[ 0 ][ 0 ] ),
                               -2.0f / ( h * m[ 1 ][ 1 ] ),
                               ( 1.0f - m[ 0 ][ 2 ] ) / m[ 0 ][ 0 ],
                               ( 1.0f + m[ 1 ][ 2 ] ) / m[ 1 ][ 1 ] );
        mBatchSaoAoRect->getGlslProg()->uniform( "uProj",       p );
        mBatchSaoAoRect->getGlslProg()->uniform( "uProjScale",  (float)h );
        mBatchSaoAoRect->draw();
        {
            gl::drawBuffer( GL_COLOR_ATTACHMENT1 );
            const gl::ScopedTextureBind scopedTextureBind( mTextureFboAo[ 0 ], 0 );
            mBatchSaoBlurRect->getGlslProg()->uniform( "uAxis", ivec2( 1, 0 ) );
            mBatchSaoBlurRect->draw();
        }
        {
            gl::drawBuffer( GL_COLOR_ATTACHMENT0 );
            const gl::ScopedTextureBind scopedTextureBind( mTextureFboAo[ 1 ], 0 );
            mBatchSaoBlurRect->getGlslProg()->uniform( "uAxis", ivec2( 0, 1 ) );
            mBatchSaoBlurRect->draw();
        }
    }

    
    //////////////////////////////////////////////////////////////////////////////////////////////
    // FINAL RENDER
    
    
    if ( mDebugMode ) {
        const gl::ScopedFramebuffer scopedFramebuffer( mFboPingPong );
        gl::drawBuffer( GL_COLOR_ATTACHMENT0 + (GLenum)ping );
        const gl::ScopedViewport scopedViewport( ivec2( 0 ), mFboPingPong->getSize() );
        const gl::ScopedMatrices scopedMatrices;
        gl::setMatricesWindow( mFboPingPong->getSize() );
        gl::disableDepthRead();
        gl::disableDepthWrite(); 
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
        {
            const gl::ScopedFramebuffer scopedFrameBuffer( mFboPingPong );
            const gl::ScopedViewport scopedViewport( ivec2( 0 ), mFboPingPong->getSize() );
            const gl::ScopedMatrices scopedMatrices;
            gl::setMatricesWindow( mFboPingPong->getSize() );
            gl::translate( mFboPingPong->getSize() / 2 );
            gl::scale( mFboPingPong->getSize() );
            gl::disableDepthRead();
            gl::disableDepthWrite();

            {
                gl::drawBuffer( GL_COLOR_ATTACHMENT0 + (GLenum)ping );
                // Blend L-buffer and AO
                const gl::ScopedTextureBind scopedTextureBind1( mTextureFboPingPong[ pong ],    0 );
                const gl::ScopedTextureBind scopedTextureBind0( mTextureFboAo[ 0 ],             1 );
                mBatchAoCompositeRect->draw();
                ping = pong;
                pong = ( ping + 1 ) % 2;
            }
        }
    }
    if( mDrawAo ) {
        const gl::ScopedFramebuffer scopedFramebuffer( mFboPingPong );
        gl::drawBuffer( GL_COLOR_ATTACHMENT0 + (GLenum)ping );
        const gl::ScopedViewport scopedViewport( ivec2( 0 ), mFboPingPong->getSize() );
        const gl::ScopedMatrices scopedMatrices;
        gl::setMatricesWindow( mFboPingPong->getSize() );
        gl::translate( mFboPingPong->getSize() / 2 );
        gl::scale( mFboPingPong->getSize() );
        gl::disableDepthRead();
        gl::disableDepthWrite();

        const gl::ScopedTextureBind scopedTextureBind( mTextureFboAo[ 0 ], 4 ); 

        mBatchDebugRect->getGlslProg()->uniform( "uSamplerAo", 4 );
        mBatchDebugRect->getGlslProg()->uniform( "uMode", 4 );
        mBatchDebugRect->draw();
    }   
    ping = pong;
    pong = ( ping + 1 ) % 2;
    {
        const gl::ScopedViewport scopedViewport( ivec2( 0 ), toPixels( getWindowSize() ) );
        const gl::ScopedMatrices scopedMatrices;
        gl::setMatricesWindow( toPixels( getWindowSize() ) );
        gl::translate( toPixels( getWindowSize() / 2 )  );
        gl::scale( toPixels( getWindowSize() ) );
        gl::disableDepthRead();
        gl::disableDepthWrite();
        const gl::ScopedTextureBind scopedTextureBind( mTextureFboPingPong[ pong ], 0 );
        // To keep bandwidth in check, we aren't using any hardware 
        // anti-aliasing (MSAA). Instead, we use FXAA as a post-process 
        // to clean up our image.
        mBatchFxaaRect->getGlslProg()->uniform( "uPixel", vec2( 1.0f ) / vec2( mFboPingPong->getSize() ) );
        mBatchFxaaRect->draw();
    }
}

void DeferredRenderer::enableDebugMode( bool enable )
{
    mDebugMode = enable;
}
} // namespace tetra

