#pragma once 


#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/Camera.h"

#include "Light.h"

namespace tetra {
class DeferredRenderer;

using DeferredRendererRef = std::shared_ptr<DeferredRenderer>;
using namespace ci;
using namespace ci::app;
using namespace ci::gl;

class DeferredRenderer {
    public:
        virtual ~DeferredRenderer(){}
        static DeferredRendererRef create(){ 
            DeferredRendererRef renderer( new DeferredRenderer() );
            return renderer;
        }

        void update();
        void enableDebugMode( bool enable = true );
        bool isDebugModeEnabled() { return mDebugMode; }

        void render( const gl::BatchRef& gBatch
                    , const gl::VboRef& indirectBuffer
                    , const uint16_t& numTetras
                    , const CameraPersp& camera );
    protected:
        DeferredRenderer();
    private:
        void createBatches();
    private:
        // Fbo's
        gl::FboRef                  mFboAo;
        gl::FboRef                  mFboGBuffer;
        gl::FboRef                  mFboLBuffer;
        gl::FboRef                  mFboPingPong;
        gl::FboRef                  mFboShadowMap;
        // Render textures
	    ci::gl::Texture2dRef		mTextureFboAo[ 2 ];
	    ci::gl::Texture2dRef		mTextureFboGBuffer[ 3 ];
	    ci::gl::Texture2dRef		mTextureFboPingPong[ 2 ];
        // Batches
        ci::gl::BatchRef			mBatchDebugRect;
        ci::gl::BatchRef			mBatchAoCompositeRect;
        ci::gl::BatchRef            mBatchGBufferPlane;
        ci::gl::BatchRef            mBatchLBufferCube;
        ci::gl::BatchRef            mBatchGBufferSphere;
        ci::gl::BatchRef            mBatchFxaaRect;
        // Lights
        std::vector<Light>			mLights;
        bool                        mDebugMode  = true;
        bool                        mDrawLights = false;
        bool                        mDrawFloor  = false;
};
} // namespace tetra
