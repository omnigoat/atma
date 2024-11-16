module;

#include <type_traits>

export module atma.policies;

import atma.meta;


export namespace atma::policies
{
	template <typename T>
	struct default_
	{
		using type = T;
	};
}

export namespace atma::policies
{
	template <template <typename> typename Retriever, typename... Policies>
	struct retrieve;

	// simple case (no default argument)
	//
	// match against the first thing that is a Retriever, or keep going
	//
	template <template <typename> typename Retriever, typename Match, typename... Policies>
	struct retrieve<Retriever, Retriever<Match>, Policies...>
	{
		using type = Match;

		// check for duplicate entries
		static_assert(!std::is_same_v<meta::nothing, default_<meta::nothing>, retrieve<Retriever, Policies...>>,
			"duplicate policies found - policies must be unique");
	};

	template <template <typename> typename Retriever, typename NotMatch, typename... Policies>
	struct retrieve<Retriever, NotMatch, Policies...>
	{
		using type = typename retrieve<Retriever, Policies...>::type;
	};

	template <template <typename> typename Retriever>
	struct retrieve<Retriever>
	{
		//static_assert(actually_false_v<Retriever<void>>, "no matching policy");
		using type = void;
	};

	// more complex case (default argument provided)
	//
	//
	template <template <typename> typename Retriever, typename Default, typename Match, typename... Policies>
	struct retrieve<Retriever, default_<Default>, Retriever<Match>, Policies...>
	{
		using type = Match;
	};

	template <template <typename> typename Retriever, typename Default, typename NotMatch, typename... Policies>
	struct retrieve<Retriever, default_<Default>, NotMatch, Policies...>
	{
		using type = typename retrieve<Retriever, default_<Default>, Policies...>::type;
	};

	template <template <typename> typename Retriever, typename Default>
	struct retrieve<Retriever, default_<Default>>
	{
		using type = Default;
	};

	template <template <typename> typename Retriever, typename... Policies>
	using retrieve_t = typename retrieve<Retriever, Policies...>::type;
}
