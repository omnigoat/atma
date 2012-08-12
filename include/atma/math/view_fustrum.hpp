//=====================================================================
//
//  Settings
//  ----------
//    This file enumerates and defines settings that certain types of
//    applications may want to/should use. It's broken up into the
//    values of those settings, and the global variables which are the
//    settings themselves.
//
//=====================================================================
//=====================================================================
// Values
// -------
//  The values have to be inside the include guards, as otherwise
//  they'd be defined multiple times.
//=====================================================================
#ifndef ATMA_MATH_SETTINGS
#define ATMA_MATH_SETTINGS
//=====================================================================
#include "atma/math/math__core.hpp"
#include "atma/math/vector.hpp"
#include "atma/math/matrix.hpp"


//=====================================================================
// Settings
// ---------
//  The settings do not need to be inside the include guards (and
//  shouldn't be), as they are all declared extern, and as such are
//  defined in atma_math.cpp.
//=====================================================================
ATMA_MATH_BEGIN
//=====================================================================
template <size_t R, size_t C, typename T> struct matrix;
//=====================================================================

	struct maths_conventions
	{
		matrix_majority m_matrix_majority;
		
		maths_conventions(matrix_majority mm = matrix_majority::row())
		 : m_matrix_majority(mm)
		{
		}
		
		template <size_t R, size_t CAR, size_t C, typename T>
		matrix<R, C, T> multiply(const matrix<R, CAR, T>& base, const matrix<CAR, C, T>& transformer)
		{
			return (m_matrix_majority == matrix_majority::row()) ? base * transformer : transformer * base;
		}
	};
	
	/*
	struct coordinate_system
	{
		// to multiply matrices by and whatnot
		//maths_conventions conventions;
		// our actual coordinate system
		vector<3> forwards, up, right;
		
		coordinate_system(maths_conventions& conventions,
			const vector<3>& forwards, const vector<3>& up, const vector<3>& right)
		 : m_conventions(conventions), m_forwards(forwards), m_up(up), m_right(right)
		{
		}
		
		matrix<4> create_transformation_matrix(const coordinate_system& cs)
		{
			return matrix<4>();
		}
		
	};
	*/
	
	
	
	class view_fustrum
	{
	private:
		// near and far clipping plane
		float m_near, m_far;
		// and a coordinate system
		//coordinate_system m_coordinate_system;
		// and maths conventions
		maths_conventions m_conventions;
		
	public:
		view_fustrum(float near_plane = 0.0f, float far_plane = 1.0f)
		 : m_near(near_plane), m_far(far_plane)
		{
		}
		
		//=====================================================================
		// Create a Perspective Projection Matrix
		//=====================================================================
		template <typename T> 
		inline matrix<4, 4, T> create_perspective_projection_matrix(T r, T l, T t, T b, T n, T f)
		{
			// mapping
			T map_n = T(m_near);
			T map_f = T(m_far);
			float fmndt = (map_f - map_n) / 2;
			float fpndt = (map_f + map_n) / 2;

			// finally, create the matrix.
			matrix<4, 4, T> mtx( matrix_type::zero() )
			
			mtx(0, 0) = 2 * n / (r - l);
			mtx(1, 1) = 2 * n / (t - b);
			mtx(2, 0) = (l + r) / (l - r);
			mtx(2, 1) = (t + b) / (b - t);
			mtx(2, 2) = (f + n) / (f - n) * fmndt + fpndt;
			mtx(2, 3) = (m_near - m_far) * n * f / (n - f)  * fmndt;
			mtx(3, 2) = 1;
			
			// I'm pretty sure this is right
			return mtx;
		}

		//=====================================================================
		// Create a perspective matrix from a FOV
		//=====================================================================
		template <typename T> 
		inline matrix<4, 4, T> create_perspective_projection_matrix_fov(T fov, T aspect, T n, T f)
		{
			T hh = fov * T(0.5) * n;
			T hw = hh * aspect;
			return createPerspectiveMatrix(-hw, hw, hh, -hh, n, f);
		}
		
		//=====================================================================
		// Create a RH View Matrix
		//=====================================================================
		template <class T>
		matrix<4, 4, T> createViewMatrix(const vector<3, T>& eye, const vector<3, T>& target, const vector<3, T>& up)
		{
			vector<3, T> z_axis = Normalize(eye - target);
			vector<3, T> x_axis = Normalize(CrossProduct(up, z_axis));
			vector<3, T> y_axis = CrossProduct(z_axis, x_axis);

			// now create the matrix
			matrix<4, 4, T> mtx;
			
			mtx(0, 0) = x_axis.x;
			mtx(0, 1) = y_axis.x;
			mtx(0, 2) = z_axis.x;
			
			mtx(1, 0) = x_axis.y;
			mtx(1, 1) = y_axis.y;
			mtx(1, 2) = z_axis.y;
			
			mtx(2, 0) = x_axis.z;
			mtx(2, 1) = y_axis.z;
			mtx(2, 2) = z_axis.z;
			
			mtx(3, 0) = -DotProduct(x_axis, eye);
			mtx(3, 1) = -DotProduct(y_axis, eye);
			mtx(3, 2) = -DotProduct(z_axis, eye);
			mtx(3, 4) = 1;
			
			// return the view matrix
			return mtx;
		}
	};


//=====================================================================
ATMA_MATH_CLOSE
//=====================================================================
#endif
//=====================================================================

