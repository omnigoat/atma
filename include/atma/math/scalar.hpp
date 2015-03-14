#pragma once

#include <atma/math/constants.hpp>

#include <cmath>


namespace atma { namespace math {

	inline void retrieve_sin_cos(float& sin, float& cos, float v)
	{
		// Map v to y in [-pi,pi], x = 2*pi*quotient + remainder.
		float quotient = one_over_two_pi * v;
		if (v >= 0.0f)
		{
			quotient = (float)((int)(quotient + 0.5f));
		}
		else
		{
			quotient = (float)((int)(quotient - 0.5f));
		}
		float y = v - two_pi * quotient;

		// Map y to [-pi/2,pi/2] with sin(y) = sin(v).
		float sign;
		if (y > pi_over_two)
		{
			y = pi - y;
			sign = -1.0f;
		}
		else if (y < -pi_over_two)
		{
			y = -pi - y;
			sign = -1.0f;
		}
		else
		{
			sign = +1.0f;
		}

		float y2 = y * y;

		// 11-degree minimax approximation
		sin = (((((-2.3889859e-08f * y2 + 2.7525562e-06f) * y2 - 0.00019840874f) * y2 + 0.0083333310f) * y2 - 0.16666667f) * y2 + 1.0f) * y;

		// 10-degree minimax approximation
		cos = sign * (((((-2.6051615e-07f * y2 + 2.4760495e-05f) * y2 - 0.0013888378f) * y2 + 0.041666638f) * y2 - 0.5f) * y2 + 1.0f);
	}


	inline float arctan(float x)
	{
		return atan(x);
	}

} }
