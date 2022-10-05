#pragma once

#include <tuple>

template<typename T>
struct DecomposedCallable
{
private:
  template<typename X>
  struct LambdaArgExtractorHelper
    : public LambdaArgExtractorHelper<decltype(&X::operator())>
  {};

  template<typename ClassT, typename R, typename... Args>
  struct LambdaArgExtractorHelper<R(ClassT::*)(Args...) const>
  {
    using args_tuple_t = std::tuple<Args...>;
  };

  template<typename ClassT, typename R, typename... Args>
  struct LambdaArgExtractorHelper<R(ClassT::*)(Args...)>
  {
    using args_tuple_t = std::tuple<Args...>;
  };

public:
  using args_tuple_t = typename LambdaArgExtractorHelper<T>::args_tuple_t;
};

template<typename R, typename... Args>
struct DecomposedCallable<R(*)(Args...)>
{
  using args_tuple_t = std::tuple<Args...>;
};

template<typename R, typename ClassT, typename... Args>
struct DecomposedCallable<R(ClassT::*)(Args...)>
{
  using args_tuple_t = std::tuple<Args...>;
};

template<typename R, typename ClassT, typename... Args>
struct DecomposedCallable<R(ClassT::*)(Args...) const>
{
  using args_tuple_t = std::tuple<Args...>;
};

template<typename R, typename ClassT, typename... Args>
struct DecomposedCallable<R(ClassT::*)(Args...) const volatile>
{
  using args_tuple_t = std::tuple<Args...>;
};

//specialisation for pure functions
template<typename R, typename... Args>
struct DecomposedCallable<R(Args...)>
{
  using args_tuple_t = std::tuple<Args...>;
};

