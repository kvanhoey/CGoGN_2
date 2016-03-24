/*******************************************************************************
* CGoGN: Combinatorial and Geometric modeling with Generic N-dimensional Maps  *
* Copyright (C) 2015, IGG Group, ICube, University of Strasbourg, France       *
*                                                                              *
* This library is free software; you can redistribute it and/or modify it      *
* under the terms of the GNU Lesser General Public License as published by the *
* Free Software Foundation; either version 2.1 of the License, or (at your     *
* option) any later version.                                                   *
*                                                                              *
* This library is distributed in the hope that it will be useful, but WITHOUT  *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License  *
* for more details.                                                            *
*                                                                              *
* You should have received a copy of the GNU Lesser General Public License     *
* along with this library; if not, write to the Free Software Foundation,      *
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.           *
*                                                                              *
* Web site: http://cgogn.unistra.fr/                                           *
* Contact information: cgogn@unistra.fr                                        *
*                                                                              *
*******************************************************************************/

#ifndef MODELISATION_ALGOS_AREA_H_
#define MODELISATION_ALGOS_AREA_H_

//#include <geometry/functions/area.h>
#include <geometry/algos/centroid.h>
#include<vector>
#include<core/basic/dart_marker.h>

namespace cgogn
{

namespace modelisation
{

template < typename MAP>
typename MAP::Vertex quadranguleFace(MAP& map, typename MAP::Face f)
{
	using Vertex = typename MAP::Vertex;
	using Edge = typename MAP::Edge;

	Dart d1 = map.phi1(f.dart);
	Dart d2 = map.phi1(map.phi1(d1));

	map.cut_face(Vertex(d1),Vertex(d2));

	map.cut_edge(Edge(map.phi_1(d1)));

	Dart x = map.phi2(map.phi_1(d1)) ;
	Dart dd = map.template phi<1111>(x) ;
	while(dd != x)
	{
		Dart next = map.template phi<11>(dd) ;
		map.cut_face(Vertex(dd), Vertex(map.phi1(x))) ;
		dd = next ;
	}

	return Vertex(map.phi2(x));	// Return a dart of the central vertex
}

template <typename VEC3, typename MAP>
void catmull_clark(MAP& map, typename MAP::template VertexAttributeHandler<VEC3>& position)
{
	using Vertex = typename MAP::Vertex;
	using Edge = typename MAP::Edge;
	using Face = typename MAP::Face;
	using Scalar = typename VEC3::Scalar;


	std::vector<Edge> initial_edges;
	std::vector<Vertex> initial_vertices;

	DartMarker<MAP> initial_edge_marker(map);

	map.foreach_cell([&] (Edge e)
	{
		initial_edges.push_back(e);
		initial_edge_marker.mark_orbit(e);
	});

	map.foreach_cell([&] (Vertex v)
	{
		initial_vertices.push_back(v);
	});

	for(Edge e: initial_edges)
	{
		Vertex v1 = Vertex(e.dart);
		Vertex v2 = Vertex(map.phi1(e.dart));
		Vertex middle = map.cut_edge(e);
		position[middle] = (position[v1] + position[v2] )/Scalar(2);
	}


	map.foreach_cell([&] (Face f)
	{
		Face ff = f;
		if (!initial_edge_marker.is_marked(f))
			ff = Face(map.phi1(f));

		initial_edge_marker.unmark_orbit(ff);

		VEC3 center = geometry::centroid<VEC3>(map,ff,position);
		Vertex vc = quadranguleFace(map,ff);
		position[vc] = center;
	});

	for(Edge e: initial_edges)
	{
		Dart e2 = map.phi2(e.dart);
		if (!map.is_boundary(e.dart) && !map.is_boundary(e2))
		{
			Vertex f1 = Vertex(map.template phi<11>(e));
			Vertex f2 = Vertex(map.phi_1(e2));
			Vertex m = Vertex(map.template phi1(e));
			position[m] = (Scalar(2)*position[m]+position[f1]+position[f2])/Scalar(4);
		}
	}

	for(Vertex v: initial_vertices)
	{
		VEC3 sum_face; // Sum_F
		sum_face.setZero();
		VEC3 sum_edge;// Sum_E
		sum_edge.setZero();


		int nb_f = 0;
		int nb_e = 0;
		Dart bound;
		map.foreach_incident_edge(v, [&] (Edge e)
		{
			nb_e++;
			sum_edge += position[Vertex(map.phi1(e))];
			if (!map.is_boundary(e))
			{
				nb_f++;
				sum_face += position[Vertex(map.template phi<11>(e))];
			}
			else
				bound = e;
		});

		if (nb_f > nb_e) // boundary case
		{
			Vertex e1 = Vertex(map.phi2(bound));
			Vertex e2 = Vertex(map.phi_1(bound));
			position[v] = Scalar(3.0/8.0)*position[v] + Scalar(1.0/4.0)*(position[e1]+position[e2]);
		}
		else
		{
			VEC3 delta = position[v]*Scalar(-3*nb_f);
			delta += sum_face + Scalar(2)*sum_edge;
			delta /= Scalar(nb_f*nb_f);
			position[v] += delta;
		}
	}

}


} // namespace geometry

} // namespace cgogn

#endif // MODELISATION_ALGOS_AREA_H_
