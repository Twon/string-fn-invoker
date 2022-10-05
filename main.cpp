#include <string>
#include <iostream>
#include <tuple>

#include "lexy_parse_helpers.h"
#include "callable_traits.h"

template<typename F>
auto wrap_fn(F&& fn) {
	return [f = std::move(fn)](std::string_view input) mutable
	{
		using args_as_tuple_t = typename  DecomposedCallable<std::decay_t<F>>::args_tuple_t;
		args_as_tuple_t res;
		auto begin = input.cbegin();
		bool got_match = false;
		if constexpr (std::tuple_size_v<args_as_tuple_t> == 0)
			got_match = (begin == input.cend()); // zero arg match -> no parse but also input must be empty
		else if constexpr (std::tuple_size_v<args_as_tuple_t> == 1)
		{
			//TODO - make more consistent... single arg Fns wont need args wrapped in parens, while multi arg Fns do...
			// single arg functions get the inner type instead of a tuple eg. int instead of std::tuple<int>
			got_match = parse_invocation(input, std::get<0>(res));
		}
		else
		{
			// we need to have a match and no leftovers in order to assume we had a correct parse
			got_match = parse_invocation(input, res);
		}
		if (got_match)
		{
			std::apply(f, std::move(res));
			return true;
		}
		return false;
	};
}

void qweqwe(std::string a, uint32_t b) {
	int z = 0;
}



int main() {
	auto ret = wrap_fn(&qweqwe);
	ret(R"#( ("test", 123)      )#");
	return 0;
}
