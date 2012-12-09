//#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#
//#
//#		Name:	calculateNormals
//#
//#		Desc:	Calculates normals given either a vertex buffer and
//#				an index buffer, or just a vertex buffer. Triangles
//#				MUST be in triangle-list form.
//#
//#		Gods:	Jonathan "goat" Runting
//#
//#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#
#ifndef ATMA_MATH_CALCULATENORMALS
#define ATMA_MATH_CALCULATENORMALS
//=====================================================================
#include <vector>
//=====================================================================
#include "atma/math/vector.hpp"
//=====================================================================
ATMA_MATH_BEGIN
//=====================================================================
	
	//=====================================================================
	// calculate normal. handy function.
	//=====================================================================
	template <typename T>
	inline vector<3, T> calculateNormal(const vector<3, T>& v1, const vector<3, T>& v2, const vector<3, T>& v3)
	{
		return Normalize(CrossProduct(v1 - v2, v1 - v3));
	}
	
	//=====================================================================
	// Caluclate normals using only a list of vertices
	//=====================================================================
	inline void calculateNormals(std::vector<math::vector<3> >& normals, const math::vector<3> * vertices_begin, const math::vector<3> * vertices_end)
	{
		// number of vertices
		size_t num_vertices = vertices_end - vertices_begin + 1;
		const math::vector<3>*& vertices = vertices_begin;
		// ergo, number of triangles
		size_t num_triangles = num_vertices / 3;
		// resize the normals vector
		normals.resize(num_vertices);

		// pre-process normals of triangles for performance boost
		std::vector<math::vector<3> > triangle_normals;
		triangle_normals.resize(num_triangles);
		for (unsigned long i = 0; i < num_triangles; i++)
		{
			triangle_normals[i] = calculateNormal(vertices[i*3], vertices[i*3+1], vertices[i*3+2]);
		}

		// loop through each vertex, finding the triangles which use it, so we
		// can process them for vertex normals
		for (unsigned long i = 0; i < num_vertices; i++)
		{
			// variables
			math::vector<3> final;
			std::vector<math::vector<3> > temp;

			// loop through triangles
			for (unsigned long j = 0; j < num_triangles; j++)
			{
				bool shared = false;
				
				// see if current vertex is used in current triangle
				if		(vertices[j * 3    ] == vertices[i])	shared = true;
				else if	(vertices[j * 3 + 1] == vertices[i])	shared = true;
				else if	(vertices[j * 3 + 2] == vertices[i])	shared = true;
				
				// if we are shared, then copy the normal to a temp array
				// and reset the shared flag
				if (shared)
				{
					temp.push_back(triangle_normals[j]);
				}
			}

			// add up all the normals
			for (unsigned long k = 0; k < temp.size(); k++) final += temp[k];

			// divide the normal by how many 
			normals[i] = Normalize(final);
		}
	}

	inline void calculateNormals(std::vector<math::vector<3> >& normals, const std::vector<math::vector<3> >& vertices)
	{
		math::calculateNormals(normals, &vertices[0], &vertices[vertices.size() - 1]);
	}

	//=====================================================================
	// Calculate normals using a vertex buffer and an index buffer
	//=====================================================================
	template <typename Idx>
	void calculateNormals(std::vector<math::vector<3> >& normals, const std::vector<math::vector<3> >& vertices, const std::vector<Idx>& indices)
	{
		//CProfile p(AS("calculate normals: all vectors"), &atma_html_profiler);
		calculateNormals(normals, &vertices[0], &vertices[0] + vertices.size(), &indices[0], &indices[0] + indices.size());
	}


	//=====================================================================
	// Calculate normals using a vertex buffer and an index buffer
	//=====================================================================
	template <typename Idx>
	void calculateNormals(std::vector<math::vector<3> >& normals, const std::vector<math::vector<3> >& vertices, Idx * indices_begin, Idx * indices_end)
	{
		//CProfile p(AS("calculate normals: indices pointers"), &atma_html_profiler);
		calculateNormals(normals, &vertices[0], &vertices[0] + vertices.size(), indices_begin, indices_end);
	}


	//=====================================================================
	// Calculate normals using a vertex buffer and an index buffer
	//=====================================================================
	template <class Idx>
	void calculateNormals(std::vector<math::vector<3> >& normals, const math::vector<3> * vertices_begin, 
	                      const math::vector<3> * vertices_end, const Idx * indices_begin, const Idx * indices_end)
	{
		//CProfile p(AS("calculate normals: vertices & indices pointers"), &atma_html_profiler);
		
		// number of vertices
		size_t num_vertices = (vertices_end - vertices_begin) + 1;
		const math::vector<3>*& vertices = vertices_begin;
		// number of indices
		size_t num_indices = (indices_end - indices_begin) + 1;
		const Idx*& indices = indices_begin;
		// number of triangles
		size_t num_triangles = num_indices / 3;
		// resize normals vector
		normals.resize(num_vertices);
		
		// pre-process normals of triangles
		std::vector<math::vector<3> > triangle_normals(num_triangles);
		for (unsigned long i = 0; i < num_triangles; i++)
		{
			triangle_normals[i] = calculateNormal(vertices[indices[i*3]], vertices[indices[i*3+1]], vertices[indices[i*3+2]]);
		}

		// loop through each vertice, finding the triangles which use it, so we
		// can process them for vertex normals
		for (unsigned long i = 0; i < num_vertices; i++)
		{
			// variables
			math::vector<3> final;
			std::vector<math::vector<3> > temp;

			// loop through triangles
			for (unsigned long j = 0; j < num_indices / 3; j++)
			{
				bool shared = false;
				
				// see if current vertex is used in current triangle
				if		(indices[j * 3] == i)		shared = true;
				else if	(indices[j * 3 + 1] == i)	shared = true;
				else if	(indices[j * 3 + 2] == i)	shared = true;
				
				// if we are shared, then copy the normal to a temp array
				// and reset the shared flag
				if (shared)
				{
					temp.push_back(triangle_normals[j]);
				}

			}

			// add up all the normals
			for (unsigned long k = 0; k < temp.size(); k++) final += temp[k];

			// divide the normal by how many 
			normals[i] = Normalize(final);
		}
	}

//=====================================================================
ATMA_MATH_CLOSE
//=====================================================================
#endif
//=====================================================================
