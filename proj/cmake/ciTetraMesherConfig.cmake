if( NOT TARGET ciTetraMesher )
	get_filename_component( ciTetraMesher_PATH "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE )
	get_filename_component( ciTetraMesher_SOURCE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../src" ABSOLUTE )
	get_filename_component( ciTetraMesher_INCLUDE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../include" ABSOLUTE )
	get_filename_component( CINDER_PATH "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE )
	
	add_library( ciTetraMesher 

				# TetraMesher
				${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/BaseMeshLoader.cpp
		  		${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/OctreeNode.cpp
     			${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/TriangleTopology.cpp
      			${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/GMSHMeshLoader.cpp
      			${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/SATriangleBoxIntersection.cpp 
			   	${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/TriMeshLoader.cpp
        		${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/GMSHMeshWriter.cpp 
               	${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/TetgenWriter.cpp
               	${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/TriMeshWriter.cpp
             	${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/Octree.cpp
               	${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/TetrahedronTopology.cpp 

				# trimesh
				${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/trimesh2/conn_comps.cc
 				${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/trimesh2/KDtree.cc
 				${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/trimesh2/subdiv.cc
 				${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/trimesh2/TriMesh_normals.cc
 				${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/trimesh2/diffuse.cc
 				${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/trimesh2/lmsmooth.cc
 				${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/trimesh2/TriMesh_bounding.cc
				${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/trimesh2/TriMesh_pointareas.cc
				${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/trimesh2/edgeflip.cc
 				${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/trimesh2/overlap.cc
 				${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/trimesh2/TriMesh_connectivity.cc
 				${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/trimesh2/TriMesh_stats.cc
 				${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/trimesh2/faceflip.cc
 				${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/trimesh2/remove.cc
				${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/trimesh2/TriMesh_curvature.cc
 				${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/trimesh2/TriMesh_tstrips.cc
 				${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/trimesh2/filter.cc
 				${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/trimesh2/reorder_verts.cc
 				${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/trimesh2/TriMesh_grid.cc
 				${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/trimesh2/ICP.cc
 				${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/trimesh2/shared.cc
 				${ciTetraMesher_SOURCE_PATH}/TetraMeshTools/trimesh2/TriMesh_io.cc 
				# CGAL
				${ciTetraMesher_SOURCE_PATH}/CGALTetrahedralize/CGALTetrahedralize.cpp
				${ciTetraMesher_SOURCE_PATH}/CGALTetrahedralize/CGALUtils.cpp		

				# final TetraMesh
				${ciTetraMesher_SOURCE_PATH}/TetraMesh.cpp		

	)

	target_compile_options( ciTetraMesher PUBLIC "-std=c++11" )

	target_include_directories( ciTetraMesher PUBLIC "${ciTetraMesher_INCLUDE_PATH}/CGALTetrahedralize" 
													 "${ciTetraMesher_INCLUDE_PATH}/TetraMeshTools"
													 "${ciTetraMesher_INCLUDE_PATH}/TetraMeshTools/trimesh2"
													 "${ciTetraMesher_INCLUDE_PATH}"
	)
	target_include_directories( ciTetraMesher BEFORE PUBLIC "${CINDER_PATH}/include" )
	target_link_libraries( ciTetraMesher PUBLIC CGAL CGAL_Core boost_thread gmp mpfr )
endif()
