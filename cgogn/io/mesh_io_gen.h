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

#ifndef IO_MESH_IO_GEN_H_
#define IO_MESH_IO_GEN_H_

#include <fstream>
#include <io/dll.h>
#include <io/c_locale.h>

namespace cgogn
{

namespace io
{

class CGOGN_IO_API MeshImportGen
{
public:
	MeshImportGen() = default;
	MeshImportGen(const MeshImportGen&) = delete;
	MeshImportGen(MeshImportGen&&) = delete;
	MeshImportGen& operator=(const MeshImportGen&) = delete;
	MeshImportGen& operator=(MeshImportGen&&) = delete;

	bool import_file(const std::string& filename);
	virtual ~MeshImportGen();
	virtual void clear() = 0;

protected:
	virtual bool import_file_impl(const std::string& filename) = 0;
};

} // namespace io

} // namespace cgogn

#endif // IO_MESH_IO_GEN_H_