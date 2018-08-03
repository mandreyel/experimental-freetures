#ifndef FREETURES_TYPE_TRAITS_HPP
#define FREETURES_TYPE_TRAITS_HPP

namespace ft {

template<class> class future;
template<class> class promise;

namespace detail {

template<class F>
struct is_callable
{
    // TODO
    constexpr static const bool value = true;
};

template<class T>
struct is_future : std::false_type {};

template<class T>
struct is_future<future<T>> : std::true_type {};

/** Determines whether the callable `F` can accept arguments of type `Args...`. */
template<class F, typename... Args>
struct accepts_args;

template<class F, typename... Args>
struct accepts_args
{
    // TODO
    constexpr static const bool value = true;
};

//------------------------------------------------------------------------------
/** Extracts the type that a future returns. */
template<class T>
struct future_result;

template<class T>
struct future_result<future<T>>
{
    using type = T;
};

//------------------------------------------------------------------------------
/**
 * Specialization for T types that are not futures. Acts as an identity
 * function.
 */
template<class T>
struct inner_result_type
{
    using type = T;
};

/** Specializations for `future<T>` types. Extracts the type T. */
template<class T>
struct inner_result_type<future<T>>
{
    using type = T;
};

//------------------------------------------------------------------------------
/**
 * Type traits for callable types used as continuations for futures.
 *
 * `result_type` is the return type of `F`.
 * `inner_result_type` is the type that `result_type` wraps if it's a future,
 * otherwise it's the same as `result_type`.
 */
template<class F, class Arg>
struct callable_traits
{
    using result_type = typename std::result_of<F(Arg)>::type;
    using inner_result_type = typename inner_result_type<result_type>::type;
    constexpr static const bool returns_future = is_future<result_type>::value;
};

} // detail
} // ft

#endif
