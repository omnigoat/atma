// implementation for atma::math::vector


//=====================================================================
// construction
//=====================================================================
vector4f::vector4f()
{
#if ATMA_MATH_USE_SSE
	xmmd_ = _mm_setzero_ps();
#else
	fpd_[0] = 0;
	fpd_[1] = 0;
	fpd_[2] = 0;
	fpd_[3] = 0;
#endif
}

vector4f::vector4f(float x, float y, float z, float w)
{
	xmmd_ = _mm_load_ps(&x);
}

template <typename OP>
vector4f::vector4f(impl::expr<vector4f, OP> const& expr)
{
#ifdef ATMA_MATH_USE_SSE
	xmmd_ = expr.xmmd();
#else
	for (
	memcpy(fpd_, expr.fpd_, 128);
#endif
}


//=====================================================================
// assignment
//=====================================================================
template <typename OP>
inline auto vector4f::operator = (const impl::expr<vector4f, OP>& e) -> vector4f&
{
#ifdef ATMA_MATH_USE_SSE
	xmmd_ = e.xmmd();
#else
	for (auto i = 0u; i != E; ++i) {
		fpd_[i] = e[i];
	}
#endif
	return *this;
}


//=====================================================================
// access
//=====================================================================
inline auto vector4f::operator [](uint32_t i) const -> float
{
	ATMA_ASSERT(i < 4);
#ifdef ATMA_MATH_USE_SSE
	return xmmd_.m128_f32[i];
#else
	return fpd_[i];
#endif
}


//=====================================================================
// computation
//=====================================================================
#if 0
inline auto vector4f::magnitude_squared() const -> float
{
	float result{};
	for (auto& x : *this)
		result += x*x;
	return result;
}
#endif

inline auto vector4f::magnitude() const -> float
{
	return std::sqrt(magnitude_squared());
}

#if 0
inline auto vector4f::normalized() const -> impl::expr<vector4f, impl::binary_oper<impl::vector_div<E,float>, vector<E,float>, float>>
{
	return impl::expr<vector4f, impl::binary_oper<impl::vector_div<E,float>, vector<E,float>, float>>(*this, magnitude());
}
#endif


//=====================================================================
// mutation
//=====================================================================
#if 0
vector4f& vector4f::operator += (const vector4f& rhs) {
	for (auto i = 0u; i != E; ++i)
		fpd_[i] += rhs.fpd_[i];
	return *this;
}

vector4f& vector4f::operator -= (const vector4f& rhs) {
	for (auto i = 0u; i != E; ++i)
		fpd_[i] -= rhs.fpd_[i];
	return *this;
}

vector4f& vector4f::operator *= (float rhs) {
	for (auto i = 0u; i != E; ++i)
		fpd_[i] *= rhs;
	return *this;
}

vector4f& vector4f::operator /= (float rhs) {
	for (auto i = 0u; i != E; ++i)
		fpd_[i] /= rhs;
	return *this;
}

vector4f& vector4f::set(uint32_t i, float n)
{
	fpd_[i] = n;
	return *this;
}
#endif

vector4f& vector4f::normalize()
{
	xmmd_ = _mm_dp_ps(xmmd_, xmmd_, 0x7f);
	return *this;
}