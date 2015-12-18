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

#ifndef UTILS_BUFFERS_H_
#define UTILS_BUFFERS_H_

#include <utils/definitions.h>
#include <core/basic/dart.h>

#include <vector>
#include <type_traits>

namespace cgogn
{

template <typename T>
class Buffers
{
	typedef T value_type;
	static const unsigned int DEFAULT_SIZE = 128u;
	static const unsigned int SHRINK_SIZE  = 1024u;

protected:

	std::vector<std::vector<T>*> buffers_;

public:

	~Buffers()
	{
		for (auto i : buffers_)
		{
			delete i;
		}
	}

	inline std::vector<T>* get_buffer()
	{
		if (buffers_.empty())
		{
			std::vector<T>* v = new std::vector<T>;
			v->reserve(DEFAULT_SIZE);
			return v;
		}

		std::vector<T>* v = buffers_.back();
		buffers_.pop_back();
		return v;
	}

	inline void release_buffer(std::vector<T>* b)
	{
		if (b->capacity() > SHRINK_SIZE)
		{
			b->resize(DEFAULT_SIZE);
			b->shrink_to_fit();
		}

		b->clear();
		buffers_.push_back(b);
	}
};

} // namespace cgogn

#endif // UTILS_BUFFERS_H_
