// Copyright (C) 2015-2021 Andrzej Krzemienski.
//
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef AK_TOOLBOX_COMPACT_OPTIONAL_HEADER_GUARD_
#define AK_TOOLBOX_COMPACT_OPTIONAL_HEADER_GUARD_

#include <cassert>
#include <utility>
#include <limits>
#include <new>
#include <type_traits>

# if defined AK_TOOLKIT_WITH_CONCEPTS
#include <concepts>
# endif

#if defined AK_TOOLBOX_NO_ARVANCED_CXX11
#  define AK_TOOLKIT_NOEXCEPT
#  define AK_TOOLKIT_IS_NOEXCEPT(E) true
#  define AK_TOOLKIT_CONSTEXPR
#  define AK_TOOLKIT_EXPLICIT_CONV
#  define AK_TOOLKIT_NOEXCEPT_AS(E)
#else
#  define AK_TOOLKIT_NOEXCEPT noexcept
#  define AK_TOOLKIT_IS_NOEXCEPT(E) noexcept(E)
#  define AK_TOOLKIT_CONSTEXPR constexpr
#  define AK_TOOLKIT_EXPLICIT_CONV explicit
#  define AK_TOOLKIT_NOEXCEPT_AS(E) noexcept(noexcept(E))
#  define AK_TOOLKIT_CONSTEXPR_NOCONST // fix in the future
#endif

#ifndef AK_TOOLKIT_LIKELY
# if defined __GNUC__
#  define AK_TOOLKIT_LIKELY(EXPR)  __builtin_expect(!!(EXPR), 1)
# else
#  define AK_TOOLKIT_LIKELY(EXPR)  (!!(EXPR))
# endif
#endif

#ifndef AK_TOOLKIT_ASSERT
# if defined NDEBUG
#  define AK_TOOLKIT_ASSERT(EXPR) void(0)
# else
#  define AK_TOOLKIT_ASSERT(EXPR) ( AK_TOOLKIT_LIKELY(EXPR) ?  void(0) : []{assert(!#EXPR);}() )
# endif
#endif

#ifndef AK_TOOLKIT_ASSERTED_EXPRESSION
# if defined NDEBUG
#  define AK_TOOLKIT_ASSERTED_EXPRESSION(CHECK, EXPR) (EXPR)
# else
#  define AK_TOOLKIT_ASSERTED_EXPRESSION(CHECK, EXPR) (AK_TOOLKIT_LIKELY(CHECK) ? (EXPR) : ([]{assert(!#CHECK);}(), (EXPR)))
# endif
#endif

#if defined __cpp_concepts && __cpp_concepts == 201507
// TODO: will conditionally support concepts
#endif

namespace ak_toolkit {
namespace markable_ns {

# if defined AK_TOOLKIT_WITH_CONCEPTS

template <typename MP>
concept mark_policy =
  requires
  {
    typename MP::value_type;
    typename MP::storage_type;
    typename MP::reference_type;
    typename MP::representation_type;
  } &&
  requires(const typename MP::representation_type & r,
           const typename MP::storage_type &        s,
           const typename MP::value_type &          cv,
                 typename MP::value_type &&         rv)
  {
    { MP::marked_value() }               -> ::std::convertible_to<typename MP::representation_type>;
    { MP::is_marked_value(r) }           -> ::std::convertible_to<bool>;

    { MP::access_value(s) }              -> ::std::same_as<typename MP::reference_type>;
    { MP::representation(s) }            -> ::std::same_as<const typename MP::representation_type &>;
    { MP::store_value(cv) }              -> ::std::convertible_to<typename MP::storage_type>;
    { MP::store_value(::std::move(rv)) } -> ::std::convertible_to<typename MP::storage_type>;
  };

# define AK_TOOLKIT_MARK_POLICY mark_policy
# else
# define AK_TOOLKIT_MARK_POLICY typename
# endif

struct default_tag{};

template <typename T, typename NT = T, typename CREF = const T&, typename REPT = NT>
struct markable_type
{
  typedef T    value_type;          // the type we claim we (optionally) store
  typedef NT   storage_type;        // the type we use for storage
  typedef CREF reference_type;      // the type that we return upon "dereference"
  typedef REPT representation_type; // the type we use to represent the marked state

  static AK_TOOLKIT_CONSTEXPR reference_type access_value(const storage_type& v) { return reference_type(v); }
  static AK_TOOLKIT_CONSTEXPR const representation_type& representation(const storage_type& v) { return v; }
  static AK_TOOLKIT_CONSTEXPR const storage_type& store_value(const value_type& v) { return v; }
  static AK_TOOLKIT_CONSTEXPR storage_type&& store_value(value_type&& v) { return std::move(v); }
};


template <typename T, T Val>
struct mark_int : markable_type<T>
{
  static AK_TOOLKIT_CONSTEXPR T marked_value() AK_TOOLKIT_NOEXCEPT { return Val; }
  static AK_TOOLKIT_CONSTEXPR bool is_marked_value(T v) AK_TOOLKIT_NOEXCEPT { return v == Val; }
};

template <typename FPT>
struct mark_fp_nan : markable_type<FPT>
{
  static AK_TOOLKIT_CONSTEXPR FPT marked_value() AK_TOOLKIT_NOEXCEPT { return std::numeric_limits<FPT>::quiet_NaN(); }
  static AK_TOOLKIT_CONSTEXPR bool is_marked_value(FPT v) AK_TOOLKIT_NOEXCEPT { return v != v; }
};

template <typename T> // requires Regular<T>
struct mark_value_init : markable_type<T>
{
  static AK_TOOLKIT_CONSTEXPR T marked_value() AK_TOOLKIT_NOEXCEPT_AS(T()) { return T(); }
  static AK_TOOLKIT_CONSTEXPR bool is_marked_value(const T& v) AK_TOOLKIT_NOEXCEPT_AS(T()) { return v == T(); }
};

template <typename T>
struct mark_stl_empty : markable_type<T>
{
  static AK_TOOLKIT_CONSTEXPR T marked_value() AK_TOOLKIT_NOEXCEPT_AS(T()) { return T(); }
  static AK_TOOLKIT_CONSTEXPR bool is_marked_value(const T& v) { return v.empty(); }
};

template <typename OT>
struct mark_optional : markable_type<typename OT::value_type, OT>
{
  typedef typename OT::value_type value_type;
  typedef OT storage_type;

  static OT marked_value() AK_TOOLKIT_NOEXCEPT { return OT(); }
  static bool is_marked_value(const OT& v) { return !v; }

  static const value_type& access_value(const storage_type& v) { return *v; }
  static storage_type store_value(const value_type& v) { return v; }
  static storage_type store_value(value_type&& v) { return std::move(v); }
};

struct mark_bool : markable_type<bool, char, bool>
{
  static AK_TOOLKIT_CONSTEXPR char marked_value() AK_TOOLKIT_NOEXCEPT { return char(2); }
  static AK_TOOLKIT_CONSTEXPR bool is_marked_value(char v) AK_TOOLKIT_NOEXCEPT { return v == 2; }

  static AK_TOOLKIT_CONSTEXPR bool access_value(const char& v) { return bool(v); }
  static AK_TOOLKIT_CONSTEXPR char store_value(const bool& v) { return v; }
};


#ifndef AK_TOOLBOX_NO_UNDERLYING_TYPE
template <typename Enum, typename std::underlying_type<Enum>::type Val>
struct mark_enum : markable_type<Enum, typename std::underlying_type<Enum>::type, Enum>
{
  typedef markable_type<Enum, typename std::underlying_type<Enum>::type, Enum> base;
  static_assert(std::is_enum<Enum>::value, "mark_enum only works with enum types");
#else
template <typename Enum, int Val>
struct mark_enum : markable_type<Enum, int, Enum>
{
  typedef markable_type<Enum, int, Enum> base;
  static_assert(sizeof(Enum) == sizeof(int), "in this compiler underlying type of enum must be int");
#endif // AK_TOOLBOX_NO_UNDERLYING_TYPE

  typedef typename base::representation_type representation_type;
  typedef typename base::storage_type        storage_type;

  static AK_TOOLKIT_CONSTEXPR representation_type marked_value() AK_TOOLKIT_NOEXCEPT { return Val; }
  static AK_TOOLKIT_CONSTEXPR bool is_marked_value(const representation_type& v) AK_TOOLKIT_NOEXCEPT { return v == Val; }

  static AK_TOOLKIT_CONSTEXPR Enum access_value(const storage_type& v) AK_TOOLKIT_NOEXCEPT { return static_cast<Enum>(v); }
  static AK_TOOLKIT_CONSTEXPR storage_type store_value(const Enum& v) AK_TOOLKIT_NOEXCEPT { return static_cast<storage_type>(v); }
};

namespace detail_ {

struct _init_nothing_tag {};

template <typename MP>
union dual_storage_union
{
  typedef typename MP::value_type value_type;
  typedef typename MP::representation_type representation_type;

  char         _nothing;
  value_type   _value;
  representation_type _marking;

  constexpr explicit dual_storage_union(_init_nothing_tag) AK_TOOLKIT_NOEXCEPT
    : _nothing() {}

  constexpr explicit dual_storage_union(representation_type && v) AK_TOOLKIT_NOEXCEPT_AS(representation_type(std::move(v)))
    : _marking(std::move(v)) {}

  constexpr explicit dual_storage_union(value_type && v) AK_TOOLKIT_NOEXCEPT_AS(value_type(std::move(v)))
    : _value(std::move(v)) {}

  constexpr explicit dual_storage_union(const value_type& v) AK_TOOLKIT_NOEXCEPT_AS(value_type(std::move(v)))
    : _value(v) {}

  ~dual_storage_union() {/* nothing here; will be properly destroyed by the owner */}
};

template <typename MVP, typename = void>
struct check_safe_dual_storage_exception_safety : ::std::true_type {};

template <typename MVP>
struct check_safe_dual_storage_exception_safety<MVP, typename MVP::is_safe_dual_storage_mark_policy>
: std::integral_constant<bool, AK_TOOLKIT_IS_NOEXCEPT(typename MVP::representation_type(MVP::marked_value()))>
{
};

} // namespace detail_

template <typename T>
struct representation_of
{
  static_assert(sizeof(T) == 0, "class template representation_of<T> needs to be specialized for your type");
};

template <typename MP>
struct dual_storage
{
  typedef typename MP::value_type value_type;
  typedef typename MP::representation_type representation_type;
  typedef typename MP::reference_type reference_type;

private:
  typedef detail_::dual_storage_union<MP> union_type;
  union_type value_;

private:
  void* address() { return static_cast<void*>(std::addressof(value_)); }
  void construct_value(const value_type& v) { ::new (address()) value_type(v); }
  void construct_value(value_type&& v) { ::new (address()) value_type(std::move(v)); }

  void change_to_value(const value_type& v)
    try {
      destroy_storage();
      construct_value(v);
    }
    catch (...)
    { // now, neither value nor no-value. We have to try to assign no-value
      construct_storage_checked();
      throw;
    }

  void change_to_value(value_type&& v)
    try {
      destroy_storage();
      construct_value(std::move(v));
    }
    catch (...)
    { // now, neither value nor no-value. We have to try to assign no-value
      construct_storage_checked();
      throw;
    }

  void construct_storage() { ::new (address()) representation_type(MP::marked_value()); }
  void construct_storage_checked() AK_TOOLKIT_NOEXCEPT { construct_storage(); }  // std::terminate() if MP::marked_value() throws

  void destroy_value() AK_TOOLKIT_NOEXCEPT { as_value().value_type::~value_type(); }
  void destroy_storage() AK_TOOLKIT_NOEXCEPT { representation().representation_type::~representation_type(); }

public:
  void clear_value() AK_TOOLKIT_NOEXCEPT { destroy_value(); construct_storage(); } // std::terminate() if MP::marked_value() throws
  bool has_value() const AK_TOOLKIT_NOEXCEPT { return !MP::is_marked_value(representation()); }

  value_type& as_value() { return value_._value; }
  const value_type& as_value() const { return value_._value; }

public:

  representation_type& representation() AK_TOOLKIT_NOEXCEPT { return value_._marking; }
  const representation_type& representation() const AK_TOOLKIT_NOEXCEPT { return value_._marking; }

  constexpr explicit dual_storage(representation_type&& mv) AK_TOOLKIT_NOEXCEPT_AS(union_type(std::move(mv)))
    : value_(std::move(mv)) {}

  constexpr explicit dual_storage(const value_type& v) AK_TOOLKIT_NOEXCEPT_AS(union_type(v))
    : value_(v) {}

  constexpr explicit dual_storage(value_type&& v) AK_TOOLKIT_NOEXCEPT_AS(union_type(std::move(v)))
    : value_(std::move(v)) {}

  dual_storage(const dual_storage& rhs) // TODO: add noexcept
    : value_(detail_::_init_nothing_tag{})
    {
      if (rhs.has_value())
        construct_value(rhs.as_value());
      else
        construct_storage();
    }

  dual_storage(dual_storage&& rhs) // TODO: add noexcept
    : value_(detail_::_init_nothing_tag{})
    {
      if (rhs.has_value())
        construct_value(std::move(rhs.as_value()));
      else
        construct_storage();
    }

  void operator=(const dual_storage& rhs)
    {
      if (has_value() && rhs.has_value())
      {
        as_value() = rhs.as_value();
      }
      else if (has_value() && !rhs.has_value())
      {
        clear_value();
      }
      else if (!has_value() && rhs.has_value())
      {
        change_to_value(rhs.as_value());
      }
    }

  void operator=(dual_storage&& rhs) // TODO: add noexcept
    {
      if (has_value() && rhs.has_value())
      {
        as_value() = std::move(rhs.as_value());
      }
      else if (has_value() && !rhs.has_value())
      {
        clear_value();
      }
      else if (!has_value() && rhs.has_value())
      {
        change_to_value(std::move(rhs.as_value()));
      }
    }

  void swap_impl(dual_storage& rhs)
  {
    using namespace std;
    if (has_value() && rhs.has_value())
    {
      swap(as_value(), rhs.as_value());
    }
    else if (has_value() && !rhs.has_value())
    {
      rhs.change_to_value(std::move(as_value()));
      clear_value();
    }
    else if (!has_value() && rhs.has_value())
    {
      change_to_value(std::move(rhs.as_value()));
      rhs.clear_value();
    }
  }

  friend void swap(dual_storage& lhs, dual_storage& rhs) { lhs.swap_impl(rhs); }

  ~dual_storage()
  {
    if (has_value())
      destroy_value();
    else
      destroy_storage();
  }
};

template <typename MPT, typename T, typename REP_T = typename representation_of<T>::type>
struct markable_dual_storage_type_unsafe
{
  static_assert(sizeof(T) == sizeof(REP_T), "representation of T has to have the same size and alignment as T");
  static_assert(std::is_standard_layout<T>::value, "T must be a Standard Layout type");
  static_assert(std::is_standard_layout<REP_T>::value, "representation of T must be a Standard Layout type");
#ifndef AK_TOOLBOX_NO_ARVANCED_CXX11
  static_assert(alignof(T) == alignof(REP_T), "representation of T has to have the same alignment as T");
#endif // AK_TOOLBOX_NO_ARVANCED_CXX11

  // TODO: in C++20: static_assert(std::is_layout_compatible_v<T, REP_T>, "representation of T has to be layout-compatible with T");

  typedef T value_type;
  typedef REP_T representation_type;
  typedef const T& reference_type;
  typedef dual_storage<MPT> storage_type;

  static  reference_type access_value(const storage_type& v)
  { return v.as_value(); }
  static  const representation_type& representation(const storage_type& v)
  { return v.representation(); }
  static  storage_type store_value(const value_type& v)
  { return storage_type(v); }
  static  storage_type store_value(value_type&& v)
  { return storage_type(std::move(v)); }
};

template <typename MPT, typename T, typename REP_T = typename representation_of<T>::type>
struct markable_dual_storage_type : markable_dual_storage_type_unsafe<MPT, T, REP_T>
{
  // The presence of this typedef is a request to check if T is nothrow move constructible
  typedef void is_safe_dual_storage_mark_policy;
};

template <typename Markable, typename MP>
struct cmp_none {};

template <typename Markable, typename MP>
struct cmp_by_storage
{
  friend auto operator==(const cmp_by_storage& l, const cmp_by_storage& r)
    -> decltype(::std::declval<const typename MP::storage_type&>() ==
	        ::std::declval<const typename MP::storage_type&>())
  { return l.m().storage_value() == r.m().storage_value(); }

  friend auto operator!=(const cmp_by_storage& l, const cmp_by_storage& r)
    -> decltype(::std::declval<const typename MP::storage_type&>() !=
	        ::std::declval<const typename MP::storage_type&>())
  { return l.m().storage_value() != r.m().storage_value(); }

private:
  const Markable& m() const noexcept { return static_cast<const Markable&>(*this); }
};

template <typename Markable, typename MP>
struct cmp_by_value_eq
{
  friend auto operator==(const cmp_by_value_eq& l, const cmp_by_value_eq& r)
    -> decltype(::std::declval<typename MP::reference_type>() ==
	        ::std::declval<typename MP::reference_type>())
  {
    return !l.m().has_value() ? !r.m().has_value() : r.m().has_value() && l.m().value() == r.m().value();
  }

  friend auto operator!=(const cmp_by_value_eq& l, const cmp_by_value_eq& r)
    -> decltype(::std::declval<typename MP::reference_type>() !=
	        ::std::declval<typename MP::reference_type>())
  {
    return !l.m().has_value() ? r.m().has_value() : !r.m().has_value() || r.m().value() != l.m().value();
  }

private:
  const Markable& m() const noexcept { return static_cast<const Markable&>(*this); }
};



template <typename MP, template <typename, typename> class CP = cmp_none>
class markable : public CP<markable<MP, CP>, MP>
{
  // The following assert is only in case a safe dual storage is used.
  // It then checks if the value_type is nothrow move constructible.
  // I cannot check it inside the dual storage, because I need a complete
  // type to determine nothrow traits.
  static_assert (detail_::check_safe_dual_storage_exception_safety<MP>::value,
                 "while building a markable type: representation of T must not throw exceptions from move constructor or when creating the marked value");
public:
  typedef typename MP::value_type value_type;
  typedef typename MP::storage_type storage_type;
  typedef typename MP::reference_type reference_type;

private:
  storage_type _storage;

public:
  AK_TOOLKIT_CONSTEXPR markable() AK_TOOLKIT_NOEXCEPT_AS(MP::marked_value())
    : _storage(MP::marked_value()) {}

  AK_TOOLKIT_CONSTEXPR explicit markable(const value_type& v)
    : _storage(MP::store_value(v)) {}

  AK_TOOLKIT_CONSTEXPR explicit markable(value_type&& v)
    : _storage(MP::store_value(std::move(v))) {}

  AK_TOOLKIT_CONSTEXPR bool has_value() const {
  	return !MP::is_marked_value(MP::representation(_storage));
  }

  AK_TOOLKIT_CONSTEXPR reference_type value() const { return AK_TOOLKIT_ASSERT(has_value()), MP::access_value(_storage); }

  AK_TOOLKIT_CONSTEXPR storage_type const& storage_value() const { return _storage; }

  void assign(value_type&& v) { _storage = MP::store_value(std::move(v)); }
  void assign(const value_type& v) { _storage = MP::store_value(v); }

  void assign_storage(storage_type&& s) { _storage = std::move(s); }
  void assign_storage(storage_type const& s) { _storage = s; }

  friend void swap(markable& lhs, markable& rhs)
  {
    using std::swap; swap(lhs._storage, rhs._storage);
  }
};

} // namespace markable_ns

using markable_ns::markable;
using markable_ns::markable_type;
using markable_ns::markable_dual_storage_type;
using markable_ns::markable_dual_storage_type_unsafe;
using markable_ns::mark_bool;
using markable_ns::mark_int;
using markable_ns::mark_fp_nan;
using markable_ns::mark_value_init;
using markable_ns::mark_optional;
using markable_ns::mark_stl_empty;
using markable_ns::mark_enum;
using markable_ns::cmp_none;
using markable_ns::cmp_by_storage;
using markable_ns::cmp_by_value_eq;


# if defined AK_TOOLKIT_WITH_CONCEPTS

using markable_ns::mark_policy;

static_assert(mark_policy<mark_bool>, "mark_policy test failed");
static_assert(mark_policy<mark_int<int, 0>>, "mark_policy test failed");
static_assert(mark_policy<mark_fp_nan<float>>, "mark_policy test failed");
static_assert(mark_policy<mark_value_init<int>>, "mark_policy test failed");

# endif

} // namespace ak_toolkit

#endif //AK_TOOLBOX_COMPACT_OPTIONAL_HEADER_GUARD_
