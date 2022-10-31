#pragma once

#include <tuple>

template <typename T> struct DecomposedCallable {
private:
	template <typename X>
	struct LambdaArgExtractorHelper
			: public LambdaArgExtractorHelper<decltype(&X::operator())> {};

	template <typename ClassT, typename R, typename... Args>
	struct LambdaArgExtractorHelper<R (ClassT::*)(Args...) const> {
    using args_tuple_t = std::tuple<Args...>;
  };

	template <typename ClassT, typename R, typename... Args>
	struct LambdaArgExtractorHelper<R (ClassT::*)(Args...)> {
    using args_tuple_t = std::tuple<Args...>;
  };

	// TODO check how this might affect the overload set
	// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p1169r4.html

public:
  using args_tuple_t = typename LambdaArgExtractorHelper<T>::args_tuple_t;
};

// TODO - examine if we can use this:
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0847r7.html
// to reduce the amount of boilerplate/partial specialisations

template <typename R, typename... Args>
struct DecomposedCallable<R (*)(Args...)> {
  using args_tuple_t = std::tuple<Args...>;
};

template <typename R, typename ClassT, typename... Args>
struct DecomposedCallable<R (ClassT::*)(Args...)> {
  using args_tuple_t = std::tuple<Args...>;
};

template <typename R, typename ClassT, typename... Args>
struct DecomposedCallable<R (ClassT::*)(Args...) const> {
  using args_tuple_t = std::tuple<Args...>;
};

template <typename R, typename ClassT, typename... Args>
struct DecomposedCallable<R (ClassT::*)(Args...) const volatile> {
  using args_tuple_t = std::tuple<Args...>;
};

// specialisation for pure functions
template <typename R, typename... Args> struct DecomposedCallable<R(Args...)> {
  using args_tuple_t = std::tuple<Args...>;
};
