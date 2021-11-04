#pragma once

import atma.types;

namespace atma
{
	struct hasher_t;

	template <typename T>
	struct hash_t
	{
		auto operator()(T const& x) const -> size_t;
		auto operator()(hasher_t& hsh, T const& x) const -> void;
	};

	struct hasher_t
	{
		hasher_t(uint64 seed = 0);

		template <typename T>
		auto operator ()(T const& t) -> hasher_t&;

		auto operator ()(void const* datav, size_t size) -> hasher_t&;

		auto result() -> uint64;

	private:
		auto mmix(uint64& h, uint64 k) const -> void
		{
			k *= m;
			k ^= k >> r;
			k *= m;
			h *= m;
			h ^= k;
		}

		void mix_tail(const uchar*& data, size_t& size)
		{
			while (size && (size < 8 || count_))
			{
				tail_ |= *data << (count_ * 8);

				++data;
				++count_;
				--size;

				if (count_ == 8)
				{
					mmix(hash_, tail_);
					tail_ = 0;
					count_ = 0;
				}
			}
		}

	private:
		static const uint64 m = 0xc6a4a7935bd1e995ull;
		static const int r = 47;

		uint64 hash_;
		uint64 tail_;
		uint64 count_;
		size_t size_;
	};




	inline auto hash(void const* key, size_t size, uint seed) -> uint64
	{
		return hasher_t(seed)(key, size).result();
	}

	template <typename T>
	inline auto hash(T const& t) -> uint64
	{
		auto hasher = hasher_t{};
		hash_t<T>{}(hasher, t);
		return hasher.result();
	}

	struct std_hash_functor_adaptor_t
	{
		template <typename T>
		auto operator ()(T const& x) const -> size_t
		{
			return hasher_t()(&x, sizeof(T)).result();
		}
	};

	template <typename T>
	inline auto hash_t<T>::operator()(T const& x) const -> size_t
	{
		return hasher_t{}(x).result();
	}

	template <typename T>
	inline auto hash_t<T>::operator()(hasher_t& hsh, T const& x) const -> void
	{
		hsh(&x, sizeof(T));
	}

	inline hasher_t::hasher_t(uint64 seed)
		: hash_(seed)
		, tail_()
		, count_()
		, size_()
	{
	}

	inline auto hasher_t::operator ()(void const* datav, size_t size) -> hasher_t&
	{
		auto data = reinterpret_cast<uchar const*>(datav);

		size_ += size;

		mix_tail(data, size);

		while (size >= 8)
		{
			uint64 k = *(uint64*)data;

			mmix(hash_, k);

			data += 8;
			size -= 8;
		}

		mix_tail(data, size);
		return *this;
	}

	inline auto hasher_t::result() -> uint64
	{
		mmix(hash_, tail_);
		mmix(hash_, size_);

		hash_ ^= hash_ >> r;
		hash_ *= m;
		hash_ ^= hash_ >> r;

		return hash_;
	}

	template <typename T>
	inline auto hasher_t::operator ()(T const& t) -> hasher_t&
	{
		hash_t<T>()(*this, t);
		return *this;
	}
}
