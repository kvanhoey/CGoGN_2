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

#ifndef CORE_CMAP_CMAP3_H_
#define CORE_CMAP_CMAP3_H_

#include <core/cmap/cmap2.h>
#include <core/basic/dart_marker.h>

namespace cgogn
{

// forward declaration of CMap3Builder_T
template <typename MAP_TRAITS>
class CMap3Builder_T;

template <typename MAP_TRAITS, typename MAP_TYPE>
class CMap3_T : public CMap2_T<MAP_TRAITS, MAP_TYPE>
{
public:

	static const int PRIM_SIZE = 1;

	using MapTraits = MAP_TRAITS;
	using MapType = MAP_TYPE;
	using Inherit = CMap2_T<MAP_TRAITS, MAP_TYPE>;
	using Self = CMap3_T<MAP_TRAITS, MAP_TYPE>;

	friend class MapBase<MAP_TRAITS, MAP_TYPE>;
	friend class CMap3Builder_T<MapTraits>;
	friend class DartMarker_T<Self>;
	friend class cgogn::DartMarkerStore<Self>;

	using CDart = Cell<Orbit::DART>;
	using Vertex = Cell<Orbit::PHI21_PHI31>;
	using Edge = Cell<Orbit::PHI2_PHI3>;
	using Face = Cell<Orbit::PHI1_PHI3>;
	using Volume = Cell<Orbit::PHI1_PHI2>;
	using Vertex2 = typename Inherit::Vertex;
	using Edge2 = typename Inherit::Edge;
	using Face2 = typename Inherit::Face;

	template <typename T>
	using ChunkArray =  typename Inherit::template ChunkArray<T>;
	template <typename T>
	using ChunkArrayContainer =  typename Inherit::template ChunkArrayContainer<T>;

	template <typename T, Orbit ORBIT>
	using AttributeHandler = typename Inherit::template AttributeHandler<T, ORBIT>;
	template <typename T>
	using VertexAttributeHandler = AttributeHandler<T, Vertex::ORBIT>;
	template <typename T>
	using EdgeAttributeHandler = AttributeHandler<T, Edge::ORBIT>;
	template <typename T>
	using FaceAttributeHandler = AttributeHandler<T, Face::ORBIT>;
	template <typename T>
	using VolumeAttributeHandler = AttributeHandler<T, Volume::ORBIT>;

	using DartMarker = typename cgogn::DartMarker<Self>;
	using DartMarkerStore = typename cgogn::DartMarkerStore<Self>;

	template <Orbit ORBIT>
	using CellMarker = typename cgogn::CellMarker<Self, ORBIT>;

protected:

	ChunkArray<Dart>* phi3_;

	inline void init()
	{
		phi3_ = this->topology_.template add_attribute<Dart>("phi3");
	}

public:

	CMap3_T() : Inherit()
	{
		init();
	}

	~CMap3_T() override
	{}

	CMap3_T(Self const&) = delete;
	CMap3_T(Self &&) = delete;
	Self& operator=(Self const&) = delete;
	Self& operator=(Self &&) = delete;

	/*******************************************************************************
	 * Low-level topological operations
	 *******************************************************************************/

protected:

	/**
	* \brief Init an newly added dart.
	* The dart is defined as a fixed point for PHI3.
	*/
	inline void init_dart(Dart d)
	{
		Inherit::init_dart(d);
		(*phi3_)[d.index] = d;
	}

	/**
	 * \brief Link dart d with dart e by an involution
	 * @param d,e the darts to link
	 *	- Before: d->d and e->e
	 *	- After:  d->e and e->d
	 */
	inline void phi3_sew(Dart d, Dart e)
	{
		cgogn_assert(phi3(d) == d);
		cgogn_assert(phi3(e) == e);
		(*phi3_)[d.index] = e;
		(*phi3_)[e.index] = d;
	}

	/**
	 * \brief Remove the phi3 link between the current dart and its linked dart
	 * @param d the dart to unlink
	 * - Before: d->e and e->d
	 * - After:  d->d and e->e
	 */
	inline void phi3_unsew(Dart d)
	{
		Dart e = phi3(d);
		(*phi3_)[d.index] = d;
		(*phi3_)[e.index] = e;
	}

	/*******************************************************************************
	 * Basic topological operations
	 *******************************************************************************/

public:

	/**
	 * \brief phi3
	 * @param d
	 * @return phi3(d)
	 */
	inline Dart phi3(Dart d) const
	{
		return (*phi3_)[d.index];
	}

	/*******************************************************************************
	 * High-level embedded and topological operations
	 *******************************************************************************/

protected:

	/**
	 * @brief create_pyramid_topo : create a pyramid whose base is n-sided
	 * @param n, the number of edges of the base
	 * @return a dart from the base
	 */
	inline Dart add_pyramid_topo(unsigned int n)
	{
		cgogn_message_assert( n >= 3u ,"The base must have at least 3 edges.");

		std::vector<Dart> m_tableVertDarts;
		m_tableVertDarts.reserve(n);

		// creation of triangles around circunference and storing vertices
		for (unsigned int i = 0u; i < n; ++i)
		{
			m_tableVertDarts.push_back(this->Inherit::Inherit::add_face_topo(3u));
		}

		// sewing the triangles
		for (unsigned int i = 0u; i < n-1u; ++i)
		{
			const Dart d = this->phi_1(m_tableVertDarts[i]);
			const Dart e = this->phi1(m_tableVertDarts[i+1]);
			this->phi2_sew(d,e);
		}

		//sewing the last with the first
		this->phi2_sew(this->phi1(m_tableVertDarts[0ul]), this->phi_1(m_tableVertDarts[n-1u]));

		//sewing the bottom face
		Dart base = this->Inherit::Inherit::add_face_topo(n);
		const Dart dres = base;
		for(unsigned int i = 0u; i < n; ++i)
		{
			this->phi2_sew(m_tableVertDarts[i], base);
			base = this->phi1(base);
		}
		//return a dart from the base
		return dres;
	}

	/**
	 * @brief create_prism_topo : create a prism whose base is n-sided
	 * @param n, the number of edges of the base
	 * @return a dart from the base
	 */
	Dart add_prism_topo(unsigned int n)
	{
		cgogn_message_assert( n >= 3u ,"The base must have at least 3 edges.");
		std::vector<Dart> m_tableVertDarts;
		m_tableVertDarts.reserve(n*2u);

		// creation of quads around circunference and storing vertices
		for (unsigned int i = 0u; i < n; ++i)
		{
			m_tableVertDarts.emplace_back(this->Inherit::Inherit::add_face_topo(4));
		}

		// storing a dart from the vertex pointed by phi1(phi1(d))
		for (unsigned int i = 0u; i < n; ++i)
		{
			m_tableVertDarts.emplace_back(this->phi1(this->phi1(m_tableVertDarts[i])));
		}

		// sewing the quads
		for (unsigned int i = 0u; i < n-1u; ++i)
		{
			const Dart d = this->phi_1(m_tableVertDarts[i]);
			const Dart e = this->phi1(m_tableVertDarts[i+1u]);
			this->phi2_sew(d,e);
		}
		//sewing the last with the first
		this->phi2_sew(this->phi1(m_tableVertDarts[0]), this->phi_1(m_tableVertDarts[n-1u]));

		//sewing the top & bottom faces
		Dart top = this->Inherit::Inherit::add_face_topo(n);
		Dart bottom = this->Inherit::Inherit::add_face_topo(n);
		const Dart dres = top;
		for(unsigned int i = 0u; i < n; ++i)
		{
			this->phi2_sew(m_tableVertDarts[i], top);
			this->phi2_sew(m_tableVertDarts[n+i], bottom);
			top = this->phi1(top);
			bottom = this->phi_1(bottom);
		}

		//return a dart from the base
		return dres;
	}

	inline void close_hole_topo(Dart d)
	{
		cgogn_message_assert(phi3(d) == d, "CMap3: close hole called on a dart that is not a phi3 fix point");
		DartMarkerStore dmarker(*this);
		DartMarkerStore boundary_marker(*this);

		std::vector<Dart> visitedFaces;	// Faces that are traversed
		visitedFaces.reserve(1024);

		visitedFaces.push_back(d);		// Start with the face of d
		dmarker.mark_orbit(Face2(d));

		unsigned int count = 0u;

		// For every face added to the list
		for(unsigned int i = 0u; i < visitedFaces.size(); ++i)
		{
			Dart it = visitedFaces[i];
			Dart f = it;

			const Dart b = this->Inherit::Inherit::add_face_topo(this->degree(Face(f)));
			boundary_marker.mark_orbit(Face2(b));
			++count;

			Dart bit = b;
			do
			{
				Dart e = this->phi3(this->phi2(f));;
				bool found = false;
				do
				{
					if (phi3(e) == e)
					{
						found = true;
						if(!dmarker.is_marked(e))
						{
							visitedFaces.push_back(e);
							dmarker.mark_orbit(Face2(e));
						}
					} else {
						if(boundary_marker.is_marked(e))
						{
							found = true;
							this->phi2_sew(e, bit);
						} else {
							e = this->phi3(this->phi2(e));
						}
					}
				} while(!found);

				phi3_sew(f, bit);
				bit = this->phi_1(bit);
				f = this->phi1(f);
			} while(f != it);
		}
	}

	/**
	 * @brief close_map : /!\ DO NOT USE /!\ Close the map removing topological holes (only for import/creation)
	 * Add volumes to the map that close every existing hole.
	 * @return the number of closed holes
	 */
	inline unsigned int close_map()
	{
		CGOGN_CHECK_CONCRETE_TYPE;
		// Search the map for topological holes (fix points of phi3)
		unsigned int nb = 0u;
		for (Dart d: (*this))
		{
			if (phi3(d) == d)
			{
				++nb;
				close_hole_topo(d);
				const Volume new_volume = phi3(d);

				if (this->template is_embedded<CDart>())
					foreach_dart_of_orbit(new_volume, [this] (Dart d)
					{
						this->new_orbit_embedding(CDart(d));
					});

				if (this->template is_embedded<Vertex2>())
					Inherit::foreach_incident_vertex(new_volume, [this] (Vertex2 v)
					{
						this->new_orbit_embedding(v);
					});

				if (this->template is_embedded<Edge2>())
					Inherit::foreach_incident_edge(new_volume, [this] (Edge2 e)
					{
						this->new_orbit_embedding(e);
					});

				if (this->template is_embedded<Face2>())
				{
					Inherit::foreach_incident_face(new_volume, [this] (Face2 f)
					{
						this->new_orbit_embedding(f);
					});
				}

				if (this->template is_embedded<Vertex>())
				{
					foreach_dart_of_orbit(new_volume, [this] (Dart wd)
					{
						this->copy_embedding<Vertex::ORBIT>(wd, this->phi1(phi3(wd)));
					});
				}

				if (this->template is_embedded<Edge>())
				{
					foreach_dart_of_orbit(new_volume, [this] (Dart wd)
					{
						this->copy_embedding<Edge::ORBIT>(wd, phi3(wd));
					});
				}

				if (this->template is_embedded<Face>())
				{
					foreach_dart_of_orbit(new_volume, [this] (Dart wd)
					{
						this->copy_embedding<Face::ORBIT>(wd, phi3(wd));
					});

				}
				if (this->template is_embedded<Volume>())
				{
					this->new_orbit_embedding(new_volume);
				}
			}
		}
		return nb;
	}

public:

	inline unsigned int degree(Face f) const
	{
		return Inherit::degree(typename Inherit::Face(f.dart));
	}

protected:

	/*******************************************************************************
	 * Orbits traversal
	 *******************************************************************************/

	template <typename FUNC>
	inline void foreach_dart_of_PHI21_PHI31(Dart d, const FUNC& f) const
	{
		DartMarkerStore marker(*this);
		const std::vector<Dart>* marked_darts = marker.get_marked_darts();

		marker.mark(d);
		for(unsigned int i = 0; i < marked_darts->size(); ++i)
		{
			f((*marked_darts)[i]);

			Dart d1 = this->phi1((*marked_darts)[i]);
			Dart d21 = this->phi2(d1); // turn in volume
			Dart d31 = phi3(d1); // change volume
			if(!marker.is_marked(d21))
				marker.mark(d21);
			if(!marker.is_marked(d31))
				marker.mark(d31);
		}
	}

	template <typename FUNC>
	inline void foreach_dart_of_PHI2_PHI3(Dart d, const FUNC& f) const
	{
		Dart it = d;
		do
		{
			f(it);
			it = this->phi2(it);
			f(it);
			it = phi3(it);
		} while (it != d);
	}

	template <typename FUNC>
	inline void foreach_dart_of_PHI23(Dart d, const FUNC& f) const
	{
		Dart it = d;
		do
		{
			f(it);
			it = phi3(this->phi2(it));
		} while (it != d);
	}

	template <typename FUNC>
	inline void foreach_dart_of_PHI1_PHI3(Dart d, const FUNC& f) const
	{
		this->foreach_dart_of_PHI1(d, [&] (Dart fd)
		{
			f(fd);
			f(phi3(fd));
		});
	}

	template <Orbit ORBIT, typename FUNC>
	inline void foreach_dart_of_orbit(Cell<ORBIT> c, const FUNC& f) const
	{
		static_assert(check_func_parameter_type(FUNC, Dart),
					  "Wrong function parameter type");

		static_assert(ORBIT == Orbit::DART || ORBIT == Orbit::PHI1 || ORBIT == Orbit::PHI2 ||
					  ORBIT == Orbit::PHI1_PHI2 || ORBIT == Orbit::PHI21 ||
					  ORBIT == Orbit::PHI1_PHI3 || ORBIT == Orbit::PHI2_PHI3 || ORBIT == Orbit::PHI21_PHI31,
					  "Orbit not supported in a CMap3");

		switch (ORBIT)
		{
			case Orbit::DART: f(c.dart); break;
			case Orbit::PHI1: this->foreach_dart_of_PHI1(c, f); break;
			case Orbit::PHI2: this->foreach_dart_of_PHI2(c, f); break;
			case Orbit::PHI1_PHI2: this->foreach_dart_of_PHI1_PHI2(c, f); break;
			case Orbit::PHI1_PHI3: foreach_dart_of_PHI1_PHI3(c, f); break;
			case Orbit::PHI2_PHI3: foreach_dart_of_PHI2_PHI3(c, f); break;
			case Orbit::PHI21: this->foreach_dart_of_PHI21(c, f); break;
			case Orbit::PHI21_PHI31: foreach_dart_of_PHI21_PHI31(c, f); break;
			default: cgogn_assert_not_reached("This orbit is not handled"); break;
		}
	}

	template <typename FUNC>
	inline void foreach_dart_of_PHI21_PHI31_until(Dart d, const FUNC& f) const
	{
		DartMarkerStore marker(*this);
		const std::vector<Dart>* marked_darts = marker.get_marked_darts();

		marker.mark(d);
		for(unsigned int i = 0; i < marked_darts->size(); ++i)
		{
			if (!f((*marked_darts)[i]))
				break;

			Dart d2 = this->phi2((*marked_darts)[i]);
			Dart d21 = this->phi1(d2); // turn in volume
			Dart d23 = phi3(d2); // change volume
			if(!marker.is_marked(d21))
				marker.mark(d21);
			if(!marker.is_marked(d23))
				marker.mark(d23);
		}
	}

	template <typename FUNC>
	inline void foreach_dart_of_PHI2_PHI3_until(Dart d, const FUNC& f) const
	{
		Dart it = d;
		do
		{
			if (!f(it))
				break;
			it = this->phi2(it);
			if (!f(it))
				break;
			it = phi3(it);
		} while (it != d);
	}

	template <typename FUNC>
	inline void foreach_dart_of_PHI23_until(Dart d, const FUNC& f) const
	{
		Dart it = d;
		do
		{
			if (!f(it))
				break;
			it = phi3(this->phi2(it));
		} while (it != d);
	}

	template <typename FUNC>
	inline void foreach_dart_of_PHI1_PHI3_until(Dart d, const FUNC& f) const
	{
		this->foreach_dart_of_PHI1_until(d, [&] (Dart fd) -> bool
		{
			if (f(fd))
				return f(phi3(fd));
			return false;
		});
	}

	template <Orbit ORBIT, typename FUNC>
	inline void foreach_dart_of_orbit_until(Cell<ORBIT> c, const FUNC& f) const
	{
		static_assert(check_func_parameter_type(FUNC, Dart),
					  "Wrong function parameter type");

		static_assert(check_func_return_type(FUNC, bool),
					  "Wrong function return type");

		static_assert(ORBIT == Orbit::DART || ORBIT == Orbit::PHI1 || ORBIT == Orbit::PHI2 ||
					  ORBIT == Orbit::PHI1_PHI2 || ORBIT == Orbit::PHI21 ||
					  ORBIT == Orbit::PHI1_PHI3 || ORBIT == Orbit::PHI2_PHI3 || ORBIT == Orbit::PHI21_PHI31,
					  "Orbit not supported in a CMap3");

		switch (ORBIT)
		{
			case Orbit::DART: f(c.dart); break;
			case Orbit::PHI1: this->foreach_dart_of_PHI1_until(c, f); break;
			case Orbit::PHI2: this->foreach_dart_of_PHI2_until(c, f); break;
			case Orbit::PHI1_PHI2: this->foreach_dart_of_PHI1_PHI2_until(c, f); break;
			case Orbit::PHI1_PHI3: foreach_dart_of_PHI1_PHI3_until(c, f); break;
			case Orbit::PHI2_PHI3: foreach_dart_of_PHI2_PHI3_until(c, f); break;
			case Orbit::PHI21: this->foreach_dart_of_PHI21_until(c, f); break;
			case Orbit::PHI21_PHI31: foreach_dart_of_PHI21_PHI31_until(c, f); break;
			default: cgogn_assert_not_reached("This orbit is not handled"); break;
		}
	}

public:

	/*******************************************************************************
	 * Incidence traversal
	 *******************************************************************************/

	template <typename FUNC>
	inline void foreach_incident_edge(Vertex v, const FUNC& f) const
	{
		static_assert(check_func_parameter_type(FUNC, Edge), "Wrong function cell parameter type");
		DartMarkerStore marker(*this);
		foreach_dart_of_orbit(v, [&] (Dart d)
		{
			if (!marker.is_marked(d))
			{
				foreach_dart_of_PHI23(d, [&marker] (Dart dd) { marker.mark(dd); });
				f(d);
			}
		});
	}

	template <typename FUNC>
	inline void foreach_incident_face(Vertex v, const FUNC& f) const
	{
		static_assert(check_func_parameter_type(FUNC, Face), "Wrong function cell parameter type");
		DartMarkerStore marker(*this);
		foreach_dart_of_orbit(v, [&] (Dart d)
		{
			if (!marker.is_marked(d))
			{
				marker.mark(d);
				marker.mark(this->phi1(phi3(d)));
				f(d);
			}
		});
	}

	template <typename FUNC>
	inline void foreach_incident_volume(Vertex v, const FUNC& f) const
	{
		static_assert(check_func_parameter_type(FUNC, Volume), "Wrong function cell parameter type");
		DartMarkerStore marker(*this);
		foreach_dart_of_orbit(v, [&] (Dart d)
		{
			if (!marker.is_marked(d))
			{
				marker.mark_orbit(Vertex2(d));
				f(d);
			}
		});
	}

	template <typename FUNC>
	inline void foreach_incident_vertex(Edge e, const FUNC& f) const
	{
		static_assert(check_func_parameter_type(FUNC, Vertex), "Wrong function cell parameter type");
		f(e.dart);
		f(this->phi2(e.dart));
	}

	template <typename FUNC>
	inline void foreach_incident_face(Edge e, const FUNC& f) const
	{
		static_assert(check_func_parameter_type(FUNC, Face), "Wrong function cell parameter type");
		foreach_dart_of_PHI23(e, f);
	}

	template <typename FUNC>
	inline void foreach_incident_volume(Edge e, const FUNC& f) const
	{
		static_assert(check_func_parameter_type(FUNC, Volume), "Wrong function cell parameter type");
		foreach_dart_of_PHI23(e, f);
	}

	template <typename FUNC>
	inline void foreach_incident_vertex(Face f, const FUNC& func) const
	{
		static_assert(check_func_parameter_type(FUNC, Vertex), "Wrong function cell parameter type");
		foreach_dart_of_orbit(Face2(f.dart), [&func](Dart v) {func(Vertex(v));});
	}

	template <typename FUNC>
	inline void foreach_incident_edge(Face f, const FUNC& func) const
	{
		static_assert(check_func_parameter_type(FUNC, Edge), "Wrong function cell parameter type");
		foreach_dart_of_orbit(Face2(f.dart), [&func](Dart e) {func(Edge(e));});
	}

	template <typename FUNC>
	inline void foreach_incident_volume(Face f, const FUNC& func) const
	{
		static_assert(check_func_parameter_type(FUNC, Volume), "Wrong function cell parameter type");
		func(f);
		func(phi3(f.dart));
	}

	template <typename FUNC>
	inline void foreach_incident_vertex(Volume v, const FUNC& f) const
	{
		static_assert(check_func_parameter_type(FUNC, Vertex), "Wrong function cell parameter type");
		Inherit::foreach_incident_vertex(v, f);
	}

	template <typename FUNC>
	inline void foreach_incident_edge(Volume v, const FUNC& f) const
	{
		static_assert(check_func_parameter_type(FUNC, Edge), "Wrong function cell parameter type");
		Inherit::foreach_incident_edge(v, f);
	}

	template <typename FUNC>
	inline void foreach_incident_face(Volume v, const FUNC& f) const
	{
		static_assert(check_func_parameter_type(FUNC, Face), "Wrong function cell parameter type");
		Inherit::foreach_incident_face(v, f);
	}

	/*******************************************************************************
	 * Adjacence traversal
	 *******************************************************************************/

	template <typename FUNC>
	inline void foreach_adjacent_vertex_through_edge(Vertex v, const FUNC& f) const
	{
		static_assert(check_func_parameter_type(FUNC, Vertex), "Wrong function cell parameter type");
		foreach_incident_edge(v, [&] (Edge e)
		{
			f(Vertex(this->phi2(e.dart)));
		});
	}

	template <typename FUNC>
	inline void foreach_adjacent_vertex_through_face(Vertex v, const FUNC& f) const
	{
		static_assert(check_func_parameter_type(FUNC, Vertex), "Wrong function cell parameter type");
		DartMarker marker_vertex(*this);
		marker_vertex.mark_orbit(v);
		foreach_incident_face(v, [&] (Face inc_face)
		{
			foreach_incident_vertex(inc_face, [&] (Vertex vertex_of_face)
			{
				if (!marker_vertex.is_marked(vertex_of_face))
				{
					marker_vertex.mark_orbit(vertex_of_face);
					f(vertex_of_face);
				}
			});
		});
	}

	template <typename FUNC>
	inline void foreach_adjacent_vertex_through_volume(Vertex v, const FUNC& f) const
	{
		static_assert(check_func_parameter_type(FUNC, Vertex), "Wrong function cell parameter type");
		DartMarker marker_vertex(*this);
		marker_vertex.mark_orbit(v);
		foreach_incident_volume(v, [&] (Volume inc_vol)
		{
			foreach_incident_vertex(inc_vol, [&] (Vertex inc_vert)
			{
				if (!marker_vertex.is_marked(inc_vert))
				{
					marker_vertex.mark_orbit(inc_vert);
					f(inc_vert);
				}
			});
		});
	}

	template <typename FUNC>
	inline void foreach_adjacent_edge_through_vertex(Edge e, const FUNC& f) const
	{
		static_assert(check_func_parameter_type(FUNC, Edge), "Wrong function cell parameter type");
		foreach_incident_vertex(e, [&] (Vertex iv)
		{
			foreach_incident_edge(iv, [&] (Edge ie)
			{
				if (ie.dart != iv.dart)
					f(ie);
			});
		});
	}

	template <typename FUNC>
	inline void foreach_adjacent_edge_through_face(Edge e, const FUNC& f) const
	{
		static_assert(check_func_parameter_type(FUNC, Edge), "Wrong function cell parameter type");
		DartMarker marker_edge(*this);
		marker_edge.mark_orbit(e);
		foreach_incident_face(e, [&] (Face inc_face)
		{
			foreach_incident_edge(inc_face, [&] (Edge inc_edge)
			{
				if (!marker_edge.is_marked(inc_edge))
				{
					marker_edge.mark_orbit(inc_edge);
					f(inc_edge);
				}
			});
		});
	}

	template <typename FUNC>
	inline void foreach_adjacent_edge_through_volume(Edge e, const FUNC& f) const
	{
		static_assert(check_func_parameter_type(FUNC, Edge), "Wrong function cell parameter type");
		DartMarker marker_edge(*this);
		marker_edge.mark_orbit(e);
		foreach_incident_volume(e, [&] (Volume inc_vol)
		{
			foreach_incident_edge(inc_vol, [&] (Edge inc_edge)
			{
				if (!marker_edge.is_marked(inc_edge))
				{
					marker_edge.mark_orbit(inc_edge);
					f(inc_edge);
				}
			});
		});
	}

	template <typename FUNC>
	inline void foreach_adjacent_face_through_vertex(Face f, const FUNC& func) const
	{
		static_assert(check_func_parameter_type(FUNC, Face), "Wrong function cell parameter type");
		DartMarker marker_face(*this);
		marker_face.mark_orbit(f);
		foreach_incident_vertex(f, [&] (Vertex v)
		{
			foreach_incident_face(f, [&](Face inc_fac)
			{
				if (!marker_face.is_marked(inc_fac))
				{
					marker_face.mark_orbit(inc_fac);
					func(inc_fac);
				}
			});
		});
	}

	template <typename FUNC>
	inline void foreach_adjacent_face_through_edge(Face f, const FUNC& func) const
	{
		static_assert(check_func_parameter_type(FUNC, Face), "Wrong function cell parameter type");
		foreach_incident_edge(f, [&] (Edge ie)
		{
			foreach_incident_face(ie, [&] (Face iface)
			{
				if (iface.dart != ie.dart)
					func(iface);
			});
		});
	}

	template <typename FUNC>
	inline void foreach_adjacent_face_through_volume(Face f, const FUNC& func) const
	{
		static_assert(check_func_parameter_type(FUNC, Face), "Wrong function cell parameter type");
		DartMarker marker_face(*this);
		marker_face.mark_orbit(f);
		foreach_incident_face(Volume(f.dart), [&] (Face inc_face)
		{
			if (!marker_face.is_marked(inc_face))
			{
				marker_face.mark_orbit((inc_face));
				func(inc_face);
			}
		});

		foreach_incident_face(Volume(phi3(f)), [&] (Face inc_face)
		{
			if (!marker_face.is_marked(inc_face))
			{
				marker_face.mark_orbit((inc_face));
				func(inc_face);
			}
		});
	}

	template <typename FUNC>
	inline void foreach_adjacent_volume_through_vertex(Volume v, const FUNC& f) const
	{
		static_assert(check_func_parameter_type(FUNC, Volume), "Wrong function cell parameter type");
		DartMarker marker_volume(*this);
		marker_volume.mark_orbit(v);
		foreach_incident_vertex(v, [&] (Vertex inc_vert)
		{
			foreach_incident_volume(inc_vert, [&](Volume inc_vol)
			{
				if (!marker_volume.is_marked(inc_vol))
				{
					marker_volume.mark_orbit(inc_vol);
					f(inc_vol);
				}
			});
		});
	}

	template <typename FUNC>
	inline void foreach_adjacent_volume_through_edge(Volume v, const FUNC& f) const
	{
		static_assert(check_func_parameter_type(FUNC, Volume), "Wrong function cell parameter type");
		DartMarker marker_volume(*this);
		marker_volume.mark_orbit(v);
		foreach_incident_edge(v, [&] (Edge inc_edge)
		{
			foreach_incident_volume(inc_edge, [&] (Volume inc_vol)
			{
				if (!marker_volume.is_marked(inc_vol))
				{
					marker_volume.mark_orbit(inc_vol);
					f(inc_vol);
				}
			});
		});
	}

	template <typename FUNC>
	inline void foreach_adjacent_volume_through_face(Volume v, const FUNC& f) const
	{
		static_assert(check_func_parameter_type(FUNC, Volume), "Wrong function cell parameter type");
		DartMarker marker_volume(*this);
		marker_volume.mark_orbit(v);
		foreach_incident_face(v, [&] (Edge inc_face)
		{
			foreach_incident_volume(inc_face, [&] (Volume inc_vol)
			{
				if (!marker_volume.is_marked(inc_vol))
				{
					marker_volume.mark_orbit(inc_vol);
					f(inc_vol);
				}
			});
		});
	}
};

template <typename MAP_TRAITS>
struct CMap3Type
{
	using TYPE = CMap3_T<MAP_TRAITS, CMap3Type<MAP_TRAITS>>;
};

template <typename MAP_TRAITS>
using CMap3 = CMap3_T<MAP_TRAITS, CMap3Type<MAP_TRAITS>>;

#if defined(CGOGN_USE_EXTERNAL_TEMPLATES) && (!defined(CORE_MAP_MAP3_CPP_))
extern template class CGOGN_CORE_API CMap3_T<DefaultMapTraits, CMap3Type<DefaultMapTraits>>;
extern template class CGOGN_CORE_API DartMarker<CMap3<DefaultMapTraits>>;
extern template class CGOGN_CORE_API DartMarkerStore<CMap3<DefaultMapTraits>>;
extern template class CGOGN_CORE_API DartMarkerNoUnmark<CMap3<DefaultMapTraits>>;
extern template class CGOGN_CORE_API CellMarker<CMap3<DefaultMapTraits>, CMap3<DefaultMapTraits>::Vertex::ORBIT>;
extern template class CGOGN_CORE_API CellMarker<CMap3<DefaultMapTraits>, CMap3<DefaultMapTraits>::Edge::ORBIT>;
extern template class CGOGN_CORE_API CellMarker<CMap3<DefaultMapTraits>, CMap3<DefaultMapTraits>::Face::ORBIT>;
extern template class CGOGN_CORE_API CellMarker<CMap3<DefaultMapTraits>, CMap3<DefaultMapTraits>::Volume::ORBIT>;
extern template class CGOGN_CORE_API CellMarkerStore<CMap3<DefaultMapTraits>, CMap3<DefaultMapTraits>::Vertex::ORBIT>;
extern template class CGOGN_CORE_API CellMarkerStore<CMap3<DefaultMapTraits>, CMap3<DefaultMapTraits>::Edge::ORBIT>;
extern template class CGOGN_CORE_API CellMarkerStore<CMap3<DefaultMapTraits>, CMap3<DefaultMapTraits>::Face::ORBIT>;
extern template class CGOGN_CORE_API CellMarkerStore<CMap3<DefaultMapTraits>, CMap3<DefaultMapTraits>::Volume::ORBIT>;
#endif // defined(CGOGN_USE_EXTERNAL_TEMPLATES) && (!defined(CORE_MAP_MAP3_CPP_))

} // namespace cgogn

#endif // CORE_CMAP_CMAP3_H_
