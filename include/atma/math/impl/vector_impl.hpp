// implementation for atma::math::vector


//=====================================================================
// construction
//=====================================================================
template <unsigned int E, typename T>
vector<E, T>::vector()
{
}

template <unsigned int E, typename T>
vector<E, T>::vector(std::initializer_list<T> elements)
{
	ATMA_ASSERT(elements.size() == E);
	std::copy(elements.begin(), elements.end(), elements_.begin());
}

template <unsigned int E, typename T>
vector<E, T>::vector(const vector<E, T>& rhs)
 : elements_(rhs.elements_)
{
}


//=====================================================================
// assignment
//=====================================================================
template <unsigned int E, typename T>
vector<E, T>& vector<E, T>::operator = (const vector<E, T>& rhs) {
	elements_ = rhs.elements_;
	return *this;
}

template <unsigned int E, typename T>
template <typename OP>
vector<E, T>& vector<E, T>::operator = (const impl::expr<vector<E, T>, OP>& e) {
	for (auto i = 0u; i != E; ++i) {
		elements_[i] = e[i];
	}
	return *this;
}


//=====================================================================
// access
//=====================================================================
template <unsigned int E, typename T>
const T& vector<E, T>::operator [](unsigned int i) const {
	ATMA_ASSERT(i < E);
	return elements_[i];
}


//=====================================================================
// computation
//=====================================================================
template <unsigned int E, typename T>
inline auto vector<E, T>::magnitude_squared() const -> T
{
	T result{};
	for (auto& x : *this)
		result += x*x;
	return result;
}

template <unsigned int E, typename T>
inline auto vector<E, T>::magnitude() const -> T
{
	return std::sqrt(magnitude_squared());
}

template <unsigned int E, typename T>
inline auto vector<E, T>::normalized() const -> impl::expr<vector<E, T>, impl::binary_oper<impl::vector_div<E,T>, vector<E,T>, T>>
{
	return impl::expr<vector<E, T>, impl::binary_oper<impl::vector_div<E,T>, vector<E,T>, T>>(*this, magnitude());
}


//=====================================================================
// mutation
//=====================================================================
template <unsigned int E, typename T>
vector<E, T>& vector<E, T>::operator += (const vector<E, T>& rhs) {
	for (auto i = 0u; i != E; ++i)
		elements_[i] += rhs.elements_[i];
	return *this;
}

template <unsigned int E, typename T>
vector<E, T>& vector<E, T>::operator -= (const vector<E, T>& rhs) {
	for (auto i = 0u; i != E; ++i)
		elements_[i] -= rhs.elements_[i];
	return *this;
}

template <unsigned int E, typename T>
vector<E, T>& vector<E, T>::operator *= (const T& rhs) {
	for (auto i = 0u; i != E; ++i)
		elements_[i] *= rhs;
	return *this;
}

template <unsigned int E, typename T>
vector<E, T>& vector<E, T>::operator /= (const T& rhs) {
	for (auto i = 0u; i != E; ++i)
		elements_[i] /= rhs;
	return *this;
}

template <unsigned int E, typename T>
auto vector<E, T>::set(unsigned int i, const T& n) -> vector<E, T>&
{
	elements_[i] = n;
	return *this;
}
