// implementation for atma::math::vector


//=====================================================================
// construction
//=====================================================================
vector4f::vector4f()
{
	memset(fpd_, 0, 4 * 128);
}

vector4f::vector4f(float x, float y, float z, float w)
{
	fpd_[0] = x;
	fpd_[1] = y;
	fpd_[2] = z;
	fpd_[3] = w;
}

vector4f::vector4f(const vector4f& rhs)
{
	memcpy(fpd_, rhs.fpd_, 128);
}


//=====================================================================
// assignment
//=====================================================================
inline auto vector4f::operator = (const vector4f& rhs) -> vector4f&
{
	memcpy(fpd_, rhs.fpd_, 128);
	return *this;
}

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
inline auto vector4f::operator [](uint32_t i) const -> T const&
{
	ATMA_ASSERT(i < E);
	return fpd_[i];
}


//=====================================================================
// computation
//=====================================================================

inline auto vector4f::magnitude_squared() const -> T
{
	T result{};
	for (auto& x : *this)
		result += x*x;
	return result;
}

inline auto vector4f::magnitude() const -> T
{
	return std::sqrt(magnitude_squared());
}

inline auto vector4f::normalized() const -> impl::expr<vector4f, impl::binary_oper<impl::vector_div<E,T>, vector<E,T>, T>>
{
	return impl::expr<vector4f, impl::binary_oper<impl::vector_div<E,T>, vector<E,T>, T>>(*this, magnitude());
}


//=====================================================================
// mutation
//=====================================================================
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

vector4f& vector4f::operator *= (const T& rhs) {
	for (auto i = 0u; i != E; ++i)
		fpd_[i] *= rhs;
	return *this;
}

vector4f& vector4f::operator /= (const T& rhs) {
	for (auto i = 0u; i != E; ++i)
		fpd_[i] /= rhs;
	return *this;
}

vector4f& vector4f::set(uint32_t i, const T& n)
{
	fpd_[i] = n;
	return *this;
}
