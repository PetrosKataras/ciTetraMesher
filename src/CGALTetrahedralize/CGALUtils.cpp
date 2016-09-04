//
//  CGALUtils.cpp
//  Tetra
//
//  Created by Petros Kataras on 2/9/16.
//
//

#include "CGALUtils.h"

#ifdef nil
    #undef nil 
#endif

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Polygon_mesh_processing/compute_normal.h>

namespace {
typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
typedef Kernel::Point_3 Point;
typedef Kernel::Vector_3 Vector;
typedef CGAL::Surface_mesh<Point> Surface_mesh;
typedef boost::graph_traits<Surface_mesh>::vertex_descriptor vertex_descriptor;
typedef boost::graph_traits<Surface_mesh>::face_descriptor   face_descriptor;
}

namespace CGALUtils {

void calculateAddTetraNormals( const std::vector<cinder::vec3>& verts, std::vector<cinder::vec3>& normals )
{
    Surface_mesh mesh;
    std::vector<vertex_descriptor> vd;
    
    for( auto v : verts ) {
        vd.push_back( mesh.add_vertex( Kernel::Point_3( v.x, v.y, v.z ) ) );
    }
    
    for( size_t i = 0; i < 4; ++i ) {
        mesh.add_face( vd[i * 3 + 0], vd[i * 3 + 1], vd[i * 3 + 2] );
    }
    
    Surface_mesh::Property_map<face_descriptor, Vector> fnormals = mesh.add_property_map<face_descriptor, Vector>("f:normals", CGAL::NULL_VECTOR).first;
    Surface_mesh::Property_map<vertex_descriptor, Vector> vnormals = mesh.add_property_map<vertex_descriptor, Vector>("v:normals", CGAL::NULL_VECTOR).first;
    
    CGAL::Polygon_mesh_processing::compute_normals(mesh,
        vnormals,
        fnormals,
        CGAL::Polygon_mesh_processing::parameters::vertex_point_map(mesh.points()).
        geom_traits(Kernel()));
    
    for( auto vd : vertices(mesh) ) {
        normals.push_back( cinder::vec3(vnormals[vd].x(), vnormals[vd].y(), vnormals[vd].z() ) );
    }
}

}