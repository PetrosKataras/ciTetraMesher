cmake_minimum_required( VERSION 2.8 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )

project( TetraApp )

get_filename_component( CINDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../.." ABSOLUTE )
get_filename_component( SAMPLE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE )

include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )

set( CINDER_VERBOSE ON )

ci_make_app( 
	SOURCES ${SAMPLE_DIR}/src/TetraApp.cpp
            ${SAMPLE_DIR}/src/DeferredRenderer/DeferredRenderer.cpp
            ${SAMPLE_DIR}/src/DeferredRenderer/Light.cpp
    INCLUDES ${SAMPLE_DIR}/src/DeferredRenderer
	CINDER_PATH ${CINDER_PATH}
	BLOCKS ciTetraMesher
)
