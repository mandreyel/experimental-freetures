#ifndef FREETURES_TYPE_TRAITS_HPP
#define FREETURES_TYPE_TRAITS_HPP

namespace ft {

template<class> class future;
template<class> class promise;
class null_tag {};

namespace detail {
//------------------------------------------------------------------------------
// From: http://talesofcpp.fusionfenix.com/post-11/true-story-call-me-maybe
template <typename T>
using always_void = void;

template <typename Expr, std::size_t Step = 0, typename Enable = void>
struct is_callable_impl
  : is_callable_impl<Expr, Step + 1>
{};

// (t1.*f)(t2, ..., tN) when f is a pointer to a member function of a class T 
// and t1 is an object of type T or a reference to an object of type T or a 
// reference to an object of a type derived from T;
template <typename F, typename T, typename ...Args>
struct is_callable_impl<F(T, Args...), 0,
    always_void<decltype(
        (std::declval<T>().*std::declval<F>())(std::declval<Args>()...)
    )>
> : std::true_type
{};

// ((*t1).*f)(t2, ..., tN) when f is a pointer to a member function of a class T 
// and t1 is not one of the types described in the previous item;
template <typename F, typename T, typename ...Args>
struct is_callable_impl<F(T, Args...), 1,
    always_void<decltype(
        ((*std::declval<T>()).*std::declval<F>())(std::declval<Args>()...)
    )>
> : std::true_type
{};

// t1.*f when N == 1 and f is a pointer to member data of a class T and t1 is an 
// object of type T or a reference to an object of type T or a reference to an 
// object of a type derived from T;
template <typename F, typename T>
struct is_callable_impl<F(T), 2,
    always_void<decltype(
        std::declval<T>().*std::declval<F>()
    )>
> : std::true_type
{};

// (*t1).*f when N == 1 and f is a pointer to member data of a class T and t1 is 
// not one of the types described in the previous item;
template <typename F, typename T>
struct is_callable_impl<F(T), 3,
    always_void<decltype(
        (*std::declval<T>()).*std::declval<F>()
    )>
> : std::true_type
{};

// f(t1, t2, ..., tN) in all other cases.
template <typename F, typename ...Args>
struct is_callable_impl<F(Args...), 4,
    always_void<decltype(
        std::declval<F>()(std::declval<Args>()...)
    )>
> : std::true_type
{};

template <typename Expr>
struct is_callable_impl<Expr, 5> : std::false_type {};

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

} // detail

//------------------------------------------------------------------------------
template<class T>
struct is_future : std::false_type {};

template<class T>
struct is_future<future<T>> : std::true_type {};

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
template <typename Expr>
struct is_callable : detail::is_callable_impl<Expr> {};

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
    using inner_result_type = typename detail::inner_result_type<result_type>::type;
    constexpr static const bool returns_future = is_future<result_type>::value;
};

//template<class F>
//struct callable_traits<F, null_tag>
//{
    //using result_type = typename std::result_of<F()>::type;
    //using inner_result_type = typename detail::inner_result_type<result_type>::type;
    //constexpr static const bool returns_future = is_future<result_type>::value;
//};

} // ft

#endif
