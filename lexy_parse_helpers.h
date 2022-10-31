#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include <lexy/action/parse.hpp>
#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>
#include <lexy/input/string_input.hpp>

template <typename...> struct parse_helper;

struct error_cb {
	using return_type = void;
	template <typename Ctx, typename Err>
	void operator()(const Ctx &, const Err &err) const {
		[[maybe_unused]] auto dist = err.position() - m_input.data();
	}
	std::string_view m_input;
};

// we need to combine a terminating rule the top level rule since we dont want
// leftovers
template <typename T> struct EoFProduction {
	static constexpr auto whitespace = lexy::dsl::ascii::space;
	static constexpr auto rule = lexy::dsl::p<parse_helper<T>> + lexy::dsl::eof;
	static constexpr auto value = parse_helper<T>::value;
};

template <typename T>
bool parse_invocation(std::string_view input, T &out_val) {
	auto a = lexy::string_input(input.data(), input.size());
	auto result = lexy::parse<EoFProduction<T>>(a, error_cb{input});
	if (!result.is_success())
		return false;
	out_val = std::move(result.value());
	return true;
}

template <> struct parse_helper<uint32_t> {
	static constexpr auto whitespace = lexy::dsl::ascii::space;
	static constexpr auto rule = lexy::dsl::integer<uint32_t>;
	static constexpr auto value = lexy::construct<uint32_t>;
};

template <> struct parse_helper<bool> {
	static constexpr auto whitespace = lexy::dsl::ascii::space;
	static constexpr auto entities =
			lexy::symbol_table<bool>.map<LEXY_SYMBOL("true")>(true).map<LEXY_SYMBOL("false")>(
					false);
	static constexpr auto rule = []() {
		auto name = lexy::dsl::identifier(lexy::dsl::ascii::alpha);
		return lexy::dsl::symbol<entities>(name);
	}();
	static constexpr auto value = lexy::forward<bool>;
};

// TODO - specialise for all the remaining fundamentals

// TODO allow for full container specialisation e.g. std::vector<T, A>
template <typename T> struct parse_helper<std::vector<T>> {
	static constexpr auto whitespace = lexy::dsl::ascii::space;
	static constexpr auto rule = lexy::dsl::square_bracketed.opt_list(lexy::dsl::p<parse_helper<T>>, lexy::dsl::sep(lexy::dsl::comma));
	static constexpr auto value = lexy::as_list<std::vector<T>>;
};

template <> struct parse_helper<std::string> {
	// TODO handle escapes:
	// https://lexy.foonathan.net/reference/callback/string/#as_string No
	// automatic whitespace handling here
	static constexpr auto rule = []() {
		auto ws = lexy::dsl::whitespace(lexy::dsl::ascii::space);
		return ws + lexy::dsl::quoted(lexy::dsl::ascii::character) + ws;
	}();
	static constexpr auto value = lexy::as_string<std::string>;
};

template <typename T> struct parse_helper<std::optional<T>> {
	static constexpr auto whitespace = lexy::dsl::ascii::space;
	static constexpr auto rule = lexy::dsl::p<parse_helper<T>> | LEXY_LIT("None");
	static constexpr auto value = lexy::callback<std::optional<T>>(
			[]() { return std::nullopt; }, [](auto &&t) { return std::move(t); });
};

template <typename First, typename... Rest>
struct parse_helper<First, Rest...> {
	static constexpr auto whitespace = lexy::dsl::ascii::space;
	static constexpr auto rule =
			lexy::dsl::p<parse_helper<First>> +
			((lexy::dsl::lit_c<','> + lexy::dsl::p<parse_helper<Rest>>)+...);
	static constexpr auto value = lexy::construct<std::tuple<First, Rest...>>;
};

template <typename... Args> struct parse_helper<std::tuple<Args...>> {
	static constexpr auto whitespace = lexy::dsl::ascii::space;
	static constexpr auto rule = []() {
		if constexpr (sizeof...(Args) == 0)
			return lexy::dsl::parenthesized(lexy::dsl::nullopt);
		else
			return lexy::dsl::parenthesized(lexy::dsl::p<parse_helper<Args...>>);
	}();
	static constexpr auto value = lexy::callback<std::tuple<Args...>>(
			[](lexy::nullopt) { return std::tuple<Args...>{}; },
			[](auto &&t) { return std::move(t); });
};

template <typename T, size_t N> struct parse_helper<std::array<T, N>> {
	static constexpr auto whitespace = lexy::dsl::ascii::space;
	static constexpr auto rule = lexy::dsl::square_bracketed(lexy::dsl::times<N>(
			lexy::dsl::p<parse_helper<T>>, lexy::dsl::sep(lexy::dsl::comma)));
	static constexpr auto value = lexy::construct<std::array<T, N>>;
};

template <typename C, typename I, typename T>
lexy::scan_result<C> loop_helper(I &inserter, T &scanner) {
	C ret;
	scanner.parse(lexy::dsl::lit_c<'{'>);
	if (!scanner)
		return lexy::scan_failed;
	while (true) {
		if (scanner.branch(lexy::dsl::lit_c<','>)) {
			if (!inserter(ret, scanner))
				return lexy::scan_failed;
			continue;
		} else if (scanner.peek(lexy::dsl::lit_c<'}'>))
			break;
		if (!inserter(ret, scanner))
			return lexy::scan_failed;
	}
	scanner.parse(lexy::dsl::lit_c<'}'>);
	if (!scanner)
		return lexy::scan_failed;
	return ret;
}

template <typename T>
struct parse_helper<std::set<T>> : lexy::scan_production<std::set<T>> {
	template <typename Context, typename Reader>
	static constexpr lexy::scan_result<std::set<T>>
	scan(lexy::rule_scanner<Context, Reader> &scanner) {
		// outer rule already skips whitespace
		auto insert = [](std::set<T> &c, lexy::rule_scanner<Context, Reader> &s) {
			auto key = s.parse(parse_helper<T>{});
			if (!s)
				return false;
			const auto res = c.insert(std::move(key.value()));
			return res.second;
		};
		return loop_helper<std::set<T>>(insert, scanner);
	}

	static constexpr auto whitespace = lexy::dsl::ascii::space;
	static constexpr auto rule = lexy::dsl::scan;
	static constexpr auto value = lexy::construct<std::set<T>>;
};

template <typename K, typename V>
struct parse_helper<std::map<K, V>> : lexy::scan_production<std::map<K, V>> {
	template <typename Context, typename Reader>
	static constexpr lexy::scan_result<std::map<K, V>>
	scan(lexy::rule_scanner<Context, Reader> &scanner) {
		// outer rule already skips whitespace
		auto insert = [](std::map<K, V> &c,
										 lexy::rule_scanner<Context, Reader> &s) {
			auto key = s.parse(parse_helper<K>{});
			if (!s)
				return false;
			s.parse(lexy::dsl::lit_c<':'>);
			if (!s)
				return false;
			auto val = s.parse(parse_helper<V>{});
			if (!s)
				return false;
			const auto res =
					c.insert({std::move(key.value()), std::move(val.value())});
			return res.second;
		};
		return loop_helper<std::map<K, V>>(insert, scanner);
	}

	static constexpr auto whitespace = lexy::dsl::ascii::space;
	static constexpr auto rule = lexy::dsl::scan;
	static constexpr auto value = lexy::construct<std::map<K, V>>;
};

template <> struct parse_helper<float> : lexy::scan_production<float> {
	//	template <typename Context, typename Reader>
	//	static constexpr lexy::scan_result<float>
	//	scan(lexy::rule_scanner<Context, Reader> &scanner) {
	//		using namespace lexy::dsl;
	//		auto sign_rule = opt(lit_c<'+'> | lit_c<'-'>);
	//		auto digits_with_opt_sign = sign_rule + digits<>;
	//		//TODO support P/p ofr power-of-2 exponents
	//		auto exp_rule = opt((lit_c<'E'> | lit_c<'e'>) >>
	// digits_with_opt_sign); 		auto fract_rule_with_existing_integral =
	// opt(lit_c<'.'> >> (opt(digits<>) + exp_rule)); 		auto
	// fract_rule_with_non_existing_integral = lit_c<'.'> + digits<decimal> +
	// exp_rule;
	//		//TODO support  +- INF/INFINITY and +- NANs
	//		//TODO support 0x(hex) representation
	//		auto full_rule = sign_rule + ((digits<> >>
	// fract_rule_with_existing_integral) | else_ >>
	// fract_rule_with_non_existing_integral); 		auto res =
	// scanner.capture(token(full_rule)); 		if (!res)
	// return lexy::scan_failed; 		const auto &val = res.value();
	//		//TODO - this is obviously not performant, it's here for
	// illustration purposes only 		return
	// stof(std::string(val.begin(), val.end()));
	//	}

	template <typename Context, typename Reader>
	static constexpr lexy::scan_result<float>
	scan(lexy::rule_scanner<Context, Reader> &scanner) {
		// cheat here since we know we're contiguous
		const char *start = scanner.begin();
		char *finish = nullptr;
		auto result = strtof(start, &finish);
		if (start == finish)
			return lexy::scan_failed;
		// TODO - this is not performant for other-than-random-access iterators
		auto dist = std::distance(start, static_cast<const char *>(finish));
		while (dist--)
			scanner.parse(lexy::dsl::token(lexy::dsl::ascii::character));
		return result;
	}

	static constexpr auto whitespace = lexy::dsl::ascii::space;
	static constexpr auto rule = lexy::dsl::scan;
	static constexpr auto value = lexy::construct<float>;
};
