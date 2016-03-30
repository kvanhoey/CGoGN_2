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

#ifndef IO_VOLUME_IMPORT_H_
#define IO_VOLUME_IMPORT_H_

#include <istream>

#include <core/utils/string.h>
#include <core/container/chunk_array_container.h>
#include <core/cmap/cmap3_builder.h>

#include <geometry/functions/orientation.h>

#include <io/c_locale.h>
#include <io/dll.h>
#include <io/mesh_io_gen.h>

#include <tinyxml2.h>

/*******************************************************************************
* CGoGN convention for ordering the indices for volume cells in the VolumeImport class (prior to the map creation)
*
*-->Tetras: any order. The orientation is checked when calling add_tetra.
*           3
*         ,/|`\
*       ,/  |  `\
*     ,/    '.   `\
*   ,/       |     `\
* ,/         |       `\
*2-----------'.--------1
* `\.         |      ,/
*    `\.      |    ,/
*       `\.   '. ,/
*          `\. |/
*             `0
*
*-->Pyramids: First the indices of the quad face, then the top. The orientation is checked when calling add_pyramid.
*               4
*             ,/|\
*           ,/ .'|\
*         ,/   | | \
*       ,/    .' | `.
*     ,/      |  '.  \
*   ,/       .'   |   \
* ,/         |    |    \
*0----------.'----3    `.
* `\        |      `\    \
*   `\     .'        `\ - \
*     `\   |           `\  \
*       `\.'            `\`
*          1----------------2
*
*-->Triangular prisms: First the indices of one of the triangular face then the indices of the opposite face (same order). The orientation is checked when calling add_triangular_prism.
*       3
*     ,/|`\
*   ,/  |  `\
* ,/    |    `\
*5------+------4
*|      |      |
*|      |      |
*|      |      |
*|      |      |
*|      |      |
*|      0      |
*|    ,/ `\    |
*|  ,/     `\  |
*|,/         `\|
*2-------------1
*
*-->Hexas: First the indices of one face then the indices of the opposite face (same order). The orientation is checked when calling add_hexa.
*7----------6
*|\         |\
*| \        | \
*|  \       |  \
*|   3------+---2
*|   |      |-- |
*4---+------5   |
* \  |       \  |
*  \ |        \ |
*   \|         \|
*    0----------1
*******************************************************************************/

namespace cgogn
{

namespace io
{

template <typename MAP_TRAITS>
class VolumeImport : public MeshImportGen
{
public:

	using Self = VolumeImport<MAP_TRAITS>;
	using Inherit = MeshImportGen;
	using Map = CMap3<MAP_TRAITS>;
	using Vertex = typename Map::Vertex;
	using Volume = typename Map::Volume;
	using Face = typename Map::Face;
	using Face2 = typename Map::Face2;

	static const uint32 CHUNK_SIZE = MAP_TRAITS::CHUNK_SIZE;

	template <typename T>
	using ChunkArray = cgogn::ChunkArray<CHUNK_SIZE, T>;
	using ChunkArrayContainer = cgogn::ChunkArrayContainer<CHUNK_SIZE, uint32>;

	template <typename T, Orbit ORBIT>
	using AttributeHandler = AttributeHandler<MAP_TRAITS, T, ORBIT>;
	using MapBuilder = cgogn::CMap3Builder_T<typename Map::MapTraits>;

	virtual ~VolumeImport() override {}

protected:
	uint32 nb_vertices_;
	uint32 nb_volumes_;

	std::vector<uint32> volumes_nb_vertices_;
	std::vector<uint32> volumes_vertex_indices_;

	ChunkArrayContainer vertex_attributes_;
	ChunkArrayContainer volume_attributes_;

public:
	VolumeImport() :
		nb_vertices_(0u)
	  ,volumes_nb_vertices_()
	  ,volumes_vertex_indices_()
	{}

	CGOGN_NOT_COPYABLE_NOR_MOVABLE(VolumeImport);

	virtual void clear() override
	{
		nb_vertices_ = 0;
		volumes_nb_vertices_.clear();
		volumes_vertex_indices_.clear();
		vertex_attributes_.remove_attributes();
		volume_attributes_.remove_attributes();
	}

	bool create_map(Map& map)
	{
		if (this->nb_vertices_ == 0u)
			return false;

		MapBuilder mbuild(map);
		map.clear_and_remove_attributes();

		mbuild.template create_embedding<Vertex::ORBIT>();
		mbuild.template swap_chunk_array_container<Vertex::ORBIT>(this->vertex_attributes_);

		typename Map::template VertexAttributeHandler<std::vector<Dart>> darts_per_vertex = map.template add_attribute<std::vector<Dart>, Vertex::ORBIT>("darts_per_vertex");

		uint32 index = 0u;
		// buffer for tempo faces (used to remove degenerated edges)
		std::vector<uint32> edgesBuffer;
		edgesBuffer.reserve(16u);

		typename Map::DartMarkerStore m(map);

		//for each volume of table
		for(uint32 i = 0u; i < this->nb_volumes_; ++i)
		{
			// store volume in buffer, removing degenated faces
			const uint32 nbv = this->volumes_nb_vertices_[i];

			edgesBuffer.clear();
			uint32 prec = std::numeric_limits<uint32>::max();
			for (uint32 j = 0u; j < nbv; ++j)
			{
				uint32 em = this->volumes_vertex_indices_[index++];
				if (em != prec)
				{
					prec = em;
					edgesBuffer.push_back(em);
				}
			}
			// update nbv if duplicated vertices have been skipped
			if (nbv != static_cast<uint32>(vertices_buffer.size())) {
				std::cerr << "Warning - Import Volume: degenerated volume detected" << std::endl;
				nbv = static_cast<uint32>(vertices_buffer.size());
			}

			if(nbv == 4u) //tetrahedral case
			{
				const Dart d = mbuild.add_pyramid_topo(3u);

				const std::array<Dart, 4> vertices_of_tetra = {d,
									   map.phi1(d),
									   map.phi_1(d),
									   map.phi_1(map.phi2(map.phi_1(d)))
									  };

				std::size_t buffer_id = 0ul;
				for (const Dart dv : vertices_of_tetra)
				{
					const unsigned emb = edgesBuffer[buffer_id++];
					mbuild.init_parent_vertex_embedding(dv,emb);

					Dart dd = dv;
					do
					{
						m.mark(dd);
						darts_per_vertex[emb].push_back(dd);
						dd = map.phi1(map.phi2(dd));
					} while(dd != dv);
				}
			}
			else if(nbv == 5u) //pyramidal case
			{
				Dart d = mbuild.add_pyramid_topo(4u);

				const std::array<Dart, 5> vertices_of_pyramid = {d,
									   map.phi1(d),
									   map.phi1(map.phi1(d)),
									   map.phi_1(d),
									   map.phi_1(map.phi2(map.phi_1(d)))
									  };

				std::size_t buffer_id = 0ul;
				for (Dart dv : vertices_of_pyramid)
				{
					const unsigned emb = edgesBuffer[buffer_id++];
					mbuild.init_parent_vertex_embedding(dv,emb);

					Dart dd = dv;
					do
					{
						m.mark(dd);
						darts_per_vertex[emb].push_back(dd);
						dd = map.phi1(map.phi2(dd));
					} while(dd != dv);
				}
			}
			else if(nbv == 6u) //prism case
			{
				Dart d = mbuild.add_prism_topo(3u);
				const std::array<Dart, 6> vertices_of_prism = {
					d,
				   map.phi1(d),
				   map.phi_1(d),
				   map.phi2(map.phi1(map.phi1(map.phi2(map.phi_1(d))))),
				   map.phi2(map.phi1(map.phi1(map.phi2(d)))),
				   map.phi2(map.phi1(map.phi1(map.phi2(map.phi1(d))))),
				};

				std::size_t buffer_id = 0ul;
				for (Dart dv : vertices_of_prism)
				{
					const uint32 emb = edgesBuffer[buffer_id++];
					mbuild.init_parent_vertex_embedding(dv,emb);

					Dart dd = dv;
					do
					{
						m.mark(dd);
						darts_per_vertex[emb].push_back(dd);
						dd = map.phi1(map.phi2(dd));
					} while(dd != dv);
				}
			}
			else if(nbv == 8u) //hexahedral case
			{
				Dart d = mbuild.add_prism_topo(4u);
				const std::array<Dart, 8> vertices_of_hexa = {
					d,
					map.phi1(d),
					map.phi1(map.phi1(d)),
					map.phi_1(d),

					map.phi2(map.phi1(map.phi1(map.phi2(map.phi_1(d))))),
					map.phi2(map.phi1(map.phi1(map.phi2(d)))),
					map.phi2(map.phi1(map.phi1(map.phi2(map.phi1(d))))),
					map.phi2(map.phi1(map.phi1(map.phi2(map.phi1(map.phi1(d))))))
				};

				std::size_t buffer_id = 0ul;
				for (Dart dv : vertices_of_hexa)
				{
					const unsigned emb = edgesBuffer[buffer_id++];
					mbuild.init_parent_vertex_embedding(dv,emb);

					Dart dd = dv;
					do
					{
						m.mark(dd);
						darts_per_vertex[emb].push_back(dd);
						dd = map.phi1(map.phi2(dd));
					} while(dd != dv);
				}
			}  //end of hexa

		}

		// utilitary function
		auto sew_volumes = [&mbuild,&map,&m](Dart w1, Dart w2)
		{
			const Dart w1_begin = w1;
			do {
				mbuild.phi3_sew(w1, w2);
				w1 = map.phi1(w1);
				w2 = map.phi_1(w2);
			} while (w1_begin != w1);
		};


		//reconstruct neighbourhood
		uint32 nbBoundaryFaces = 0u;
		map.foreach_dart([&] (Dart d)
		{
			if (m.is_marked(d))
			{
				Dart good_dart;

				// 1st step : for every dart of the face we try to find a valid phi3 candidate. If we can't it's a boundary face.
				{
					Dart d_it = d;
					do
					{
						const std::vector<Dart>& vec = darts_per_vertex[Vertex(map.phi1(d_it))];
						for(auto it = vec.begin(); it != vec.end() && good_dart.is_nil(); ++it)
						{
							if(map.get_embedding(Vertex(map.phi1(*it))) == map.get_embedding(Vertex(d_it)) &&
									map.get_embedding(Vertex(map.phi_1(*it))) == map.get_embedding(Vertex(map.phi1(map.phi1(d_it)))))
							{
								good_dart = *it;
							}
						}
						d_it = map.phi1(d_it);
					} while (good_dart.is_nil() && (d_it != d));
					d = map.phi_1(d_it);
				}

				if (!good_dart.is_nil()) //not a boundary faces
				{
					const uint32 degD = map.codegree(Face(d));
					const uint32 degGD = map.codegree(Face(good_dart));

					if(degD == degGD) // normal case : the two opposite faces have the same degree
					{
						sew_volumes(d, good_dart);
						m.unmark_orbit(Face(d));
					}
					else
					{
						// there is one face of degree 4 and one face of degree 3.
						if(degD > degGD) // face of d is quad
						{
							const Dart another_d = map.phi1(map.phi1(d));
							const std::vector<Dart>& vec = darts_per_vertex[Vertex(map.phi_1(d))];

							Dart another_good_dart;
							for(auto it = vec.begin(); it != vec.end() && another_good_dart.is_nil(); ++it)
							{
								if(map.get_embedding(Vertex(map.phi1(*it))) == map.get_embedding(Vertex(another_d)) &&
										map.get_embedding(Vertex(map.phi_1(*it))) == map.get_embedding(Vertex(map.phi1(map.phi1(another_d)))))
								{
									another_good_dart = *it ;
								}
							}

							// we add a stamp volume between the faces
							const Dart d_quad = mbuild.add_stamp_volume_topo();
							{
								Dart q1_it = d;
								Dart q2_it = map.phi_1(d_quad);
								do {
									mbuild.init_parent_vertex_embedding(q2_it, map.get_embedding(Vertex(q1_it)));
									q1_it = map.phi1(q1_it);
									q2_it = map.phi_1(q2_it);
								} while (q1_it != d);
							}

							sew_volumes(d, map.phi1(map.phi1(d_quad)));
							m.unmark_orbit(Face(d));

							sew_volumes(good_dart, map.phi2(map.phi1(map.phi1(d_quad))));
							m.unmark_orbit(Face(good_dart));

							if(!another_good_dart.is_nil())
							{
								sew_volumes(another_good_dart, map.phi2(d_quad));
								m.unmark_orbit(Face(another_good_dart));
							} else
							{
								m.unmark_orbit(Face2(map.phi2(d_quad)));
								++nbBoundaryFaces;
							}
						}
						else { // // face of d is tri
							const Dart another_dart = map.phi_1(d);
							std::vector<Dart>& vec = darts_per_vertex[Vertex(d)];

							Dart another_good_dart;
							for(auto it = vec.begin(); it != vec.end() && another_good_dart.is_nil(); ++it)
							{
								if(map.get_embedding(Vertex(map.phi1(*it))) == map.get_embedding(Vertex(another_dart)) &&
										map.get_embedding(Vertex(map.phi_1(*it))) == map.get_embedding(Vertex(map.phi1(map.phi1(good_dart)))))
								{
									another_good_dart = *it ;
								}
							}

							const Dart d_quad = mbuild.add_stamp_volume_topo();
							{
								Dart q1_it = good_dart;
								Dart q2_it = d_quad;
								do {
									mbuild.init_parent_vertex_embedding(q2_it, map.get_embedding(Vertex(q1_it)));
									q1_it = map.phi1(q1_it);
									q2_it = map.phi_1(q2_it);
								} while (q1_it != good_dart);
							}

							sew_volumes(d_quad, map.phi_1(good_dart));
							m.unmark_orbit(Face(good_dart));


							sew_volumes(d, map.phi2(map.phi_1(d_quad)));
							m.unmark_orbit(Face(d));

							if (!another_good_dart.is_nil())
							{
								sew_volumes(another_good_dart, map.phi1(map.phi2(map.phi1(d_quad))));
								m.unmark_orbit(Face(another_good_dart));
							} else {
								m.unmark_orbit(Face2(map.phi1(map.phi2(map.phi1(d_quad)))));
								++nbBoundaryFaces;
							}
						}
					}
				}
				else
				{
					m.unmark_orbit(Face2(d));
					++nbBoundaryFaces;
				}
			}
		});


		if (nbBoundaryFaces > 0)
		{
			mbuild.close_map();
			std::cout << CGOGN_FUNC << ": Map closed with " << nbBoundaryFaces << " boundary face(s)." << std::endl;
		}

		unsigned int nb_vert_dart_marking = 0u;
		map.template foreach_cell<FORCE_DART_MARKING>([&nb_vert_dart_marking](Vertex v){++nb_vert_dart_marking;});

		if (this->nb_vertices_ != nb_vert_dart_marking)
			map.template enforce_unique_orbit_embedding<Vertex::ORBIT>();

		if (this->volume_attributes_.get_nb_attributes() > 0)
		{
			mbuild.template create_embedding<Volume::ORBIT>();
			mbuild.template swap_chunk_array_container<Volume::ORBIT>(this->volume_attributes_);
		}

		return true;
	}

	template<typename VEC3>
	void add_hexa(ChunkArray<VEC3>const& pos,uint32 p0, uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, bool check_orientation)
	{
		if (check_orientation)
			this->reoriente_hexa(pos, p0, p1, p2, p3, p4, p5, p6, p7);
		this->volumes_nb_vertices_.push_back(8u);
		this->volumes_vertex_indices_.push_back(p0);
		this->volumes_vertex_indices_.push_back(p1);
		this->volumes_vertex_indices_.push_back(p2);
		this->volumes_vertex_indices_.push_back(p3);
		this->volumes_vertex_indices_.push_back(p4);
		this->volumes_vertex_indices_.push_back(p5);
		this->volumes_vertex_indices_.push_back(p6);
		this->volumes_vertex_indices_.push_back(p7);
	}
	template<typename VEC3>
	inline void reoriente_hexa(ChunkArray<VEC3>const& pos, uint32& p0, uint32& p1, uint32& p2, uint32& p3, uint32& p4, uint32& p5, uint32& p6, uint32& p7)
	{
		if (geometry::test_orientation_3D(pos[p4], pos[p0],pos[p1],pos[p2]) == geometry::Orientation3D::OVER)
		{
			std::swap(p0, p3);
			std::swap(p1, p2);
			std::swap(p4, p7);
			std::swap(p5, p6);
		}
	}

	template<typename VEC3>
	void add_tetra(ChunkArray<VEC3>const& pos,uint32 p0, uint32 p1, uint32 p2, uint32 p3, bool check_orientation)
	{
		if (check_orientation)
			this->reoriente_tetra(pos,p0,p1,p2,p3);
		this->volumes_nb_vertices_.push_back(4u);
		this->volumes_vertex_indices_.push_back(p0);
		this->volumes_vertex_indices_.push_back(p1);
		this->volumes_vertex_indices_.push_back(p2);
		this->volumes_vertex_indices_.push_back(p3);
	}

	template<typename VEC3>
	inline void reoriente_tetra(ChunkArray<VEC3>const& pos, uint32& p0, uint32& p1, uint32& p2, uint32& p3)
	{
		if (geometry::test_orientation_3D(pos[p0], pos[p1],pos[p2],pos[p3]) == geometry::Orientation3D::OVER)
			std::swap(p1, p2);
	}

	template<typename VEC3>
	void add_pyramid(ChunkArray<VEC3>const& pos,uint32 p0, uint32 p1, uint32 p2, uint32 p3, uint32 p4, bool check_orientation)
	{
		this->volumes_nb_vertices_.push_back(5u);
		if (check_orientation)
			this->reoriente_pyramid(pos,p0,p1,p2,p3,p4);
		this->volumes_vertex_indices_.push_back(p0);
		this->volumes_vertex_indices_.push_back(p1);
		this->volumes_vertex_indices_.push_back(p2);
		this->volumes_vertex_indices_.push_back(p3);
		this->volumes_vertex_indices_.push_back(p4);
	}

	template<typename VEC3>
	inline void reoriente_pyramid(ChunkArray<VEC3>const& pos, uint32& p0, uint32& p1, uint32& p2, uint32& p3, uint32& p4)
	{
		if (geometry::test_orientation_3D(pos[p4], pos[p0],pos[p1],pos[p2]) == geometry::Orientation3D::OVER)
			std::swap(p1, p3);
	}

	template<typename VEC3>
	void add_triangular_prism(ChunkArray<VEC3>const& pos,uint32 p0, uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, bool check_orientation)
	{
		if (check_orientation)
			this->reoriente_triangular_prism(pos,p0,p1,p2,p3,p4,p5);
		this->volumes_nb_vertices_.push_back(6u);
		this->volumes_vertex_indices_.push_back(p0);
		this->volumes_vertex_indices_.push_back(p1);
		this->volumes_vertex_indices_.push_back(p2);
		this->volumes_vertex_indices_.push_back(p3);
		this->volumes_vertex_indices_.push_back(p4);
		this->volumes_vertex_indices_.push_back(p5);
	}

	template<typename VEC3>
	inline void reoriente_triangular_prism(ChunkArray<VEC3>const& pos, uint32& p0, uint32& p1, uint32& p2, uint32& p3, uint32& p4, uint32& p5)
	{
		if (geometry::test_orientation_3D(pos[p3], pos[p0],pos[p1],pos[p2]) == geometry::Orientation3D::OVER)
		{
			std::swap(p1,p2);
			std::swap(p4,p5);
		}
	}


};

#if defined(CGOGN_USE_EXTERNAL_TEMPLATES) && (!defined(IO_VOLUME_IMPORT_CPP_))
extern template class CGOGN_IO_API VolumeImport<DefaultMapTraits>;
#endif // defined(CGOGN_USE_EXTERNAL_TEMPLATES) && (!defined(IO_VOLUME_IMPORT_CPP_))

} // namespace io

} // namespace cgogn

#endif // IO_VOLUME_IMPORT_H_
