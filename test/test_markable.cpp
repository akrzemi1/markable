// Copyright (C) 2015 - 2021, Andrzej Krzemienski.
//
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../include/ak_toolkit/markable.hpp"
#include <cassert>
#include <utility>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <set>



#if defined AK_TOOLBOX_USING_BOOST || defined AK_TOOLKIT_USING_BOOST
#include <boost/optional.hpp>
#endif

using namespace ak_toolkit;

template <typename T>
void ignore(T&&) {}

void test_value_ctor()
{
  {
    typedef markable< mark_int<int, -1> > opt_int;
    static_assert (sizeof(opt_int) == sizeof(int), "size waste");
    static_assert (!std::is_convertible<int, opt_int>(), "markable<T> must not be convertible from T");

    opt_int oi_, oiN1(-1), oi0(0), oi1(1);
	  assert (oi_.representation_value() == -1);
    assert (!oi_.has_value());
    assert (!oiN1.has_value());
    assert ( oi0.has_value());
    assert ( oi1.has_value());

    assert (oi0.value() == 0);
    assert (oi1.value() == 1);
  }
  {
    typedef markable< mark_int<int, 0> > opt_int;
    static_assert (sizeof(opt_int) == sizeof(int), "size waste");
    static_assert (!std::is_convertible<int, opt_int>(), "markable<T> must not be convertible from T");

    opt_int oi_, oiN1(-1), oi0(0), oi1(1);
    assert (!oi_.has_value());
    assert ( oiN1.has_value());
    assert (!oi0.has_value());
    assert ( oi1.has_value());

    assert (oiN1.value() == -1);
    assert (oi1.value() == 1);
  }
}


struct string_marked_value : markable_type<std::string>
{
  typedef std::string representation_type;
  static std::string marked_value() { return std::string("\0\0", 2); }
  static bool is_marked_value(const std::string& v) { return v == std::string("\0\0", 2); }
};

void test_string_traits()
{
  typedef markable<string_marked_value> opt_str;
  static_assert (sizeof(opt_str) == sizeof(std::string), "size waste");

  opt_str os_, os00(std::string("\0\0", 2)), os0(std::string("\0")), osA(std::string("A"));
  assert (!os_.has_value());
  assert (!os00.has_value());
  assert ( os0.has_value());
  assert ( osA.has_value());
}

struct mark_first_empty : markable_type< std::string, std::pair<bool, std::string> >
{
  static storage_type marked_value() { return storage_type(false, "anything"); }
  static bool is_marked_value(const storage_type& v) { return v.first == false; }

  static const value_type& access_value(const storage_type& v) { return v.second; }
  static storage_type store_value(const value_type& v) { return storage_type(true, v); }
  static storage_type store_value(value_type&& v) { return storage_type(true, std::move(v)); }
};

void test_custom_storage()
{
  typedef markable<mark_first_empty> opt_str;
  opt_str os_, os0(std::string("\0")), osA(std::string("A"));

  assert (!os_.has_value());

  assert (os0.has_value());
  assert (os0.value() == "");

  assert (osA.has_value());
  assert (osA.value() == "A");

  swap (os_, os0);

  assert (os_.has_value());
  assert (os_.value() == "");
  assert (!os0.has_value());
}

void test_bool_storage()
{
  typedef markable<mark_bool> opt_bool;
  static_assert (sizeof(opt_bool) == 1, "size waste");

  opt_bool ob_, obT(true), obF(false);
  assert (!ob_.has_value());

  assert (obT.has_value());
  assert (obT.value() == true);

  assert (obF.has_value());
  assert (obF.value() == false);
}

void test_representation_value()
{
  typedef markable< mark_int<int, -1> > opt_int;
  opt_int oi_, oiN1(-1), oi0(0), oi1(1);

  assert ( oi_.representation_value() == -1);
  assert (oiN1.representation_value() == -1);
  assert ( oi0.representation_value() ==  0);
  assert ( oi1.representation_value() ==  1);
}

void test_mark_fp_nan()
{
  typedef markable< mark_fp_nan<double> > opt_double;
  opt_double o_, o1 (1.0), oNan (0.0/0.0);
  assert (!o_.has_value());
  assert ( o1.has_value());
  assert (!oNan.has_value());

  assert (o1.value() == 1.0);

  double v = o_.representation_value();
  assert (v != v);

  v = o1.representation_value();
  assert (v == 1.0);
  assert (v == v);

  v = oNan.representation_value();
  assert (v != v);
}

void test_mark_value_init()
{
  {
    typedef markable<mark_value_init<int>> opt_t;
    opt_t o_, o1 (1), oE(0);

    assert (!o_.has_value());
    assert ( o1.has_value());
    assert (!oE.has_value());

    assert (o1.value() == 1);

    assert (o_.representation_value() == 0);
    assert (o1.representation_value() == 1);
    assert (oE.representation_value() == 0);
  }
  {
    typedef markable<mark_value_init<std::string>> opt_t;
    opt_t o_, o1 (std::string("one")), oE ((std::string()));

    assert (!o_.has_value());
    assert ( o1.has_value());
    assert (!oE.has_value());

    assert (o1.value() == "one");

    assert (o_.representation_value() == "");
    assert (o1.representation_value() == "one");
    assert (oE.representation_value() == "");
  }
}

void test_true_or_novalue()
{
  using flag = markable<mark_int<bool, false>>;
  flag off_{}, false_{false}, true_{true};

  assert (!off_.has_value());
  assert (!false_.has_value());
  assert (true_.has_value());

  assert(equal_by_value{}(off_, false_));
  assert(equal_by_representation{}(off_, false_));

  flag f;
  f.assign_representation(false);
  assert (!f.has_value());
  bool b = f.representation_value();
  assert(!b);
}

int objects_created = 0;
int objects_destroyed = 0;
int objects_created_with_value = 0;

void reset_globals()
{
  objects_created = 0;
  objects_destroyed = 0;
  objects_created_with_value = 0;
}

struct Cont
{
  bool empty_;
  explicit Cont(bool e = true) : empty_(e) { ++objects_created_with_value; }
  bool empty() const { return empty_; }
  bool operator==(const Cont& rhs) const { return empty_ == rhs.empty_; }
};

void test_mark_stl_empty()
{
  reset_globals();
  {
    typedef markable<mark_stl_empty<Cont>> opt_t;
    opt_t o_, o1 (Cont(false)), oE(Cont(true));

    assert (objects_created_with_value == 3);
    assert (!o_.has_value());
    assert ( o1.has_value());
    assert (!oE.has_value());
    assert (objects_created_with_value == 3);

    assert (o_.representation_value() == Cont(true));
    assert (o1.representation_value() == Cont(false));
    assert (oE.representation_value() == Cont(true));
  }
  {
    typedef markable<mark_stl_empty<std::string>> opt_t;
    opt_t o_, o1 (std::string("one")), oE ((std::string()));

    assert (!o_.has_value());
    assert ( o1.has_value());
    assert (!oE.has_value());

    assert (o1.value() == "one");

    assert (o_.representation_value() == "");
    assert (o1.representation_value() == "one");
    assert (oE.representation_value() == "");
  }
  assert(objects_created == objects_destroyed);
}

enum class Dir { N, E, S, W };

void test_mark_enum()
{
  typedef markable<mark_enum<Dir, -1>> opt_dir;
  opt_dir o_, oN(Dir::N), oW(Dir::W);

  assert (!o_.has_value());
  assert ( oN.has_value());
  assert ( oW.has_value());

  assert (oN.value() == Dir::N);
  assert (oW.value() == Dir::W);

  assert (o_.representation_value() == -1);
  assert (oN.representation_value() ==  0);
  assert (oW.representation_value() ==  3);
}


void test_assignment()
{
  typedef markable<mark_enum<Dir, -1>> opt_dir;
  opt_dir od;
  assert(!od.has_value());

  od.assign(Dir::W);
  assert(od.has_value());
  assert(od.value() == Dir::W);

  od.assign_representation(-1);
  assert(!od.has_value());

  od.assign_representation(0);
  assert(od.has_value());
  assert(od.value() == Dir::N);
}


#if defined AK_TOOLKIT_USING_BOOST
void test_optional_as_storage()
{
  typedef markable<mark_optional<boost::optional<int>>> opt_int;
  opt_int oi_, oiN1(-1), oi0(0);
  assert (!oi_.has_value());

  assert (oiN1.has_value());
  assert (oiN1.value() == -1);

  assert (oi0.has_value());
  assert (oi0.value() == 0);
}
#endif


class range
{
  int min_, max_;
  bool invariant() const { return min_ <= max_; }

public:
  range(int min, int max) : min_(min), max_(max)
  {
    assert (invariant());
    ++objects_created;
  }

  range(range const& rhs) : min_(rhs.min_), max_(rhs.max_)
  {
    assert (invariant());
    assert (rhs.invariant());
    ++objects_created;
  }

  range& operator=(range const&) = default;

  int min() const
  {
    assert (invariant());
    return min_;
  }

  int max() const
  {
    assert (invariant());
    return max_;
  }

  friend bool operator==(const range& l, const range& r)
  {
    return l.min_ == r.min_ && l.max_ == r.max_;
  }

  ~range()
  {
    assert (invariant());
    ++objects_destroyed;
  }
};

struct range_representation
{
  int min_, max_;
};

struct mark_range : markable_dual_storage_type<mark_range, range, range_representation>
{
  static representation_type marked_value() AK_TOOLKIT_NOEXCEPT { return {0, -1}; }
  static bool is_marked_value(const representation_type& v) { return v.min_ > v.max_; }
};

struct range2_members
{
  int min_, max_;
};

class range2 : private range2_members
{
  bool invariant() const { return min_ <= max_; }

public:
  range2(int min, int max) : range2_members{min, max}
  {
    assert (invariant());
    ++objects_created;
  }

  range2(range2 const& rhs) : range2_members(static_cast<range2_members const&>(rhs))
  {
    assert (invariant());
    assert (rhs.invariant());
    ++objects_created;
  }

  range2& operator=(range2 const&) = default;

  int min() const
  {
    assert (invariant());
    return min_;
  }

  int max() const
  {
    assert (invariant());
    return max_;
  }

  friend bool operator==(const range2& l, const range2& r)
  {
    return l.min_ == r.min_ && l.max_ == r.max_;
  }

  ~range2()
  {
    assert (invariant());
    ++objects_destroyed;
  }
};

struct range2_representation : range2_members
{
  range2_representation() AK_TOOLKIT_NOEXCEPT : range2_members{0, -1} {};
};


namespace ak_toolkit { namespace markable_ns {
  template<> struct representation_of<range2>
  {
    typedef range2_representation type;
  };
}}

struct mark_range2 : markable_dual_storage_type<mark_range2, range2>
{
  static representation_type marked_value() AK_TOOLKIT_NOEXCEPT { return {}; }
  static bool is_marked_value(const representation_type& v) { return v.min_ > v.max_; }
};

template <typename T, typename MP>
void test_mark_dual_storage()
{
  reset_globals();
  {
    typedef markable<MP> opt_range;
    const T t0(0, 0), tM(0, 10);
    opt_range ot_, ot0 (t0), otM(tM);
    assert (!ot_.has_value());
    assert ( ot0.has_value());
    assert ( otM.has_value());
    assert(objects_created == 4);

    assert (ot0.value() == t0);
    assert (otM.value() == tM);

    opt_range otM2 = otM;
    assert (otM.has_value());
    assert (otM.value() == tM);
    assert(objects_created == 5);

    ot_ = otM2;
    assert (ot_.has_value());
    assert (ot_.value() == tM);
    assert(objects_created == 6);
    assert(objects_destroyed == 0);

    ot_ = {};
    assert (!ot_.has_value());
    assert ( otM.has_value());
    assert(objects_created == 6);
    assert(objects_destroyed == 1);

    swap(ot_, otM);
    assert ( ot_.has_value());
    assert (!otM.has_value());
    assert (ot_.value() == tM);
    assert(objects_created == 7);
    assert(objects_destroyed == 2);
  }
  assert(objects_created == objects_destroyed);
}

void test_mark_dual_storage_1() { test_mark_dual_storage<range, mark_range>(); }
void test_mark_dual_storage_2() { test_mark_dual_storage<range2, mark_range2>(); }

struct mark_negative : markable_type<int>
{
  static int marked_value() { return -2; }
  static bool is_marked_value(const int& v) { return v < 0; }
};


template <typename MP, typename OP, typename EQ>
void assert_correct_eq_properties
(markable<MP, OP> const& a, markable<MP, OP> const& b, EQ eq)
{
  assert( eq(a, b) ==  eq(b, a) );
  assert( eq(a, a) == true );
  assert( eq(b, b) == true );
}

template <typename MP, typename OP>
void assert_correct_relation_between_relops
  (markable<MP, OP> const& a, markable<MP, OP> const& b)
{
  assert( (a == b) ==  (b == a) );
  assert( (a != b) ==  (b != a) );
  assert( (a != b) == !(a == b) );
  assert( (b != a) == !(b == a) );

  assert( (a < b) == (b > a) );
  assert( (a < b) == !(a >= b) );
  assert( (a < b) == !(b <= a) );

  assert( (a == b) == (a >= b && a <= b) );
  assert( (a == b) == (!(a < b) && !(a > b)) );
  assert( (a <= b) == ((a < b) || (a == b)) );
  assert( (a < b) == (!(a > b) && !(a == b)) );
  assert( (a != b) == (a > b) || (b > a) );

  if (a < b) assert(a != b);
  if (a > b) assert(a != b);
}

void test_order_by_value()
{
  {
    using opt_int = markable<mark_int<int, 0>, order_by_value>;
    opt_int n1 {-1}, oo {}, p1{+1};

    assert_correct_relation_between_relops(n1, n1);
    assert_correct_relation_between_relops(p1, p1);
    assert_correct_relation_between_relops(oo, oo);
    assert_correct_relation_between_relops(n1, oo);
    assert_correct_relation_between_relops(p1, n1);
    assert_correct_relation_between_relops(oo, p1);

    assert(n1 == n1);
    assert(p1 == p1);
    assert(oo == oo);

    // no-value is smaller than any value
    assert(oo < n1);
    assert(n1 < p1);
    assert(oo < p1);

    std::hash<int> hash_int;
    std::hash<opt_int> hash_opt_int;
    assert(hash_opt_int(n1) == hash_int(-1));
    assert(hash_opt_int(p1) == hash_int(+1));
    assert(hash_opt_int(oo) == std::hash<int>::result_type{});
  }
  {
    using opt_int = markable<mark_negative, order_by_value>;
    opt_int xx{}, n1{-1}, _0 {0}, p1{+1};

    {
      assert(xx.representation_value() != n1.representation_value());
      assert(!xx.has_value());
      assert(!n1.has_value());
    }

    assert_correct_relation_between_relops(xx, xx);
    assert_correct_relation_between_relops(n1, n1);
    assert_correct_relation_between_relops(_0, _0);
    assert_correct_relation_between_relops(p1, p1);

    assert_correct_relation_between_relops(xx, n1);
    assert_correct_relation_between_relops(xx, _0);
    assert_correct_relation_between_relops(xx, p1);
    assert_correct_relation_between_relops(n1, _0);
    assert_correct_relation_between_relops(n1, p1);
    assert_correct_relation_between_relops(_0, p1);

    assert(xx == xx);
    assert(_0 == _0);
    assert(n1 == n1);
    assert(p1 == p1);

    // different no-values are always equal
    assert(xx == n1);

    // no-value is smaller than any value
    assert(xx < _0);
    assert(xx < p1);
    assert(n1 < _0);
    assert(n1 < p1);
    assert(_0 < p1);

    std::hash<int> hash_int;
    std::hash<opt_int> hash_opt_int;
    assert(hash_opt_int(xx) == std::hash<int>::result_type{});
    assert(hash_opt_int(n1) == std::hash<int>::result_type{});
    assert(hash_opt_int(_0) == hash_int(0));
    assert(hash_opt_int(p1) == hash_int(+1));
  }
}

void test_order_by_representation()
{
  {
    using opt_int = markable<mark_int<int, 0>, order_by_representation>;
    opt_int n1 {-1}, _0{}, p1 {+1};

    assert_correct_relation_between_relops(n1, n1);
    assert_correct_relation_between_relops(p1, p1);
    assert_correct_relation_between_relops(_0, _0);
    assert_correct_relation_between_relops(n1, _0);
    assert_correct_relation_between_relops(p1, n1);
    assert_correct_relation_between_relops(_0, p1);

    assert(n1 == n1);
    assert(p1 == p1);
    assert(_0 == _0);

    // no-value is smaller than any value
    assert(n1 < _0);
    assert(n1 < p1);
    assert(_0 < p1);

    std::hash<int> hash_int;
    std::hash<opt_int> hash_opt_int;
    assert(hash_opt_int(n1) == hash_int(-1));
    assert(hash_opt_int(p1) == hash_int(+1));
    assert(hash_opt_int(_0) == hash_int(0));
  }
  {
    using opt_int = markable<mark_negative, order_by_representation>;
    opt_int xx{}, _0{0}, p1{+1};
    // opt_int n1{-1}; // illegal: it would create value-less markable with storage
                       // value different than -2 (MP::marked_value())

    assert_correct_relation_between_relops(xx, xx);
    assert_correct_relation_between_relops(_0, _0);
    assert_correct_relation_between_relops(p1, p1);

    assert_correct_relation_between_relops(xx, _0);
    assert_correct_relation_between_relops(xx, p1);
    assert_correct_relation_between_relops(_0, p1);

    assert(xx == xx);
    assert(_0 == _0);
    assert(p1 == p1);

    // no-value is smaller than any value
    assert(xx < _0);
    assert(xx < p1);
    assert(_0 < p1);

    std::hash<int> hash_int;
    std::hash<opt_int> hash_opt_int;
    assert(hash_opt_int(xx) == hash_int(-2));
    assert(hash_opt_int(_0) == hash_int(0));
    assert(hash_opt_int(p1) == hash_int(+1));
  }
}

void test_default_markable()
{
  default_markable<int> i_, i0(0), i1(1), iM(std::numeric_limits<int>::max());
  assert(!i_.has_value());
  assert(!iM.has_value());
  assert(i0.has_value());
  assert(i1.has_value());

  default_markable<float> f_, f0(0.0f);
  assert(!f_.has_value());
  assert(f0.has_value());

  enum class Dir { N, E, S, W };

  default_markable<Dir> d_, dN(Dir::N);
  assert(!d_.has_value());
  assert(dN.has_value());

  default_markable<int*> p_;
  assert(!p_.has_value());
  assert(p_.representation_value() == nullptr);

  default_markable<bool> b_, bT(true), bF(false);
  assert(!b_.has_value());
  assert(bT.has_value());
  assert(bF.has_value());
}

void test_manual_cmp()
{
  struct mark_small_negative_int : markable_type<int>
  {
    constexpr static int marked_value() { return -1; }
    constexpr static bool is_marked_value(const int& v) {
      return v <= -1 && v >= -9;
    }
  };
  using opt_int = markable<mark_int<int, -1>>;
  opt_int iNeg{-10}, iNul2{-2}, iNul1{}, i0{0}, iPos{1};

  assert_correct_eq_properties(iNeg, iNeg, equal_by_value{});
  assert_correct_eq_properties(iNeg, iNul2, equal_by_value{});
  assert_correct_eq_properties(iNeg, iNul1, equal_by_value{});
  assert_correct_eq_properties(iNeg, i0, equal_by_value{});
  assert_correct_eq_properties(iNeg, iPos, equal_by_value{});

  assert(!equal_by_representation{}(iNul2, iNul1));

  assert(less_by_value{}(iNeg, i0));
  assert(less_by_value{}(iNeg, iPos));
  assert(less_by_value{}(iNul1, iNeg));
  assert(!less_by_value{}(iNeg, iNul1));
  assert(!less_by_value{}(iNeg, iNeg));
  assert(!less_by_value{}(iNul1, iNul1));

  std::vector<opt_int> v;
  std::sort(v.begin(), v.end(), less_by_value{});
  std::sort(v.begin(), v.end(), less_by_representation{});
  std::unordered_set<opt_int, hash_by_representation> s;
  std::set<opt_int, less_by_value> osv;
  std::set<opt_int, less_by_representation> osr;
}

// Test if comparison work for dual storage

class TOD // Time Of Day
{
  int _minutes;

public:
  bool invariant() const { return 0 <= _minutes && _minutes < 1440; }
  explicit TOD(int min) : _minutes(min) { assert(invariant()); }
  friend bool operator<(const TOD& l, const TOD& r) { return l._minutes < r._minutes; }
  friend bool operator==(const TOD& l, const TOD& r) { return l._minutes == r._minutes; }
  friend bool operator!=(const TOD& l, const TOD& r) { return !(l == r); }
};

struct TOD_representation
{
  int minutes;
};

struct mark_TOD : markable_dual_storage_type<mark_TOD, TOD, TOD_representation>
{
  static representation_type marked_value() AK_TOOLKIT_NOEXCEPT { return {-1}; }
  static bool is_marked_value(const representation_type& v) { return v.minutes < 0; }
};

struct TOD_representation_relops
{
  int minutes;

  friend bool operator<(const TOD_representation_relops& l, const TOD_representation_relops& r) { return l.minutes < r.minutes; }
  friend bool operator==(const TOD_representation_relops& l, const TOD_representation_relops& r) { return l.minutes == r.minutes; }
};

struct mark_TOD_cmp : markable_dual_storage_type<mark_TOD_cmp, TOD, TOD_representation_relops>
{
  static representation_type marked_value() AK_TOOLKIT_NOEXCEPT { return {-1}; }
  static bool is_marked_value(const representation_type& v) { return v.minutes < 0; }
};


void test_dual_storage_ordering()
{
  {
    using opt_TOD = markable<mark_TOD, order_by_value>;
    opt_TOD _0{TOD{0}}, _1{TOD{1}}, xx;

    assert_correct_relation_between_relops(_0, _0);
    assert_correct_relation_between_relops(_1, _1);
    assert_correct_relation_between_relops(xx, xx);
    assert_correct_relation_between_relops(_0, _1);
    assert_correct_relation_between_relops(_1, xx);
    assert_correct_relation_between_relops(xx, _0);

    assert(_0 == _0);
    assert(_1 == _1);
    assert(xx == xx);

    // no-value is smaller than any value
    assert(xx < _0);
    assert(_0 < _1);
    assert(xx < _1);
  }
  {
    using opt_TOD = markable<mark_TOD_cmp, order_by_representation>;
    opt_TOD _0{TOD{0}}, _1{TOD{1}}, xx;

    assert_correct_relation_between_relops(_0, _0);
    assert_correct_relation_between_relops(_1, _1);
    assert_correct_relation_between_relops(xx, xx);
    assert_correct_relation_between_relops(_0, _1);
    assert_correct_relation_between_relops(_1, xx);
    assert_correct_relation_between_relops(xx, _0);

    assert(_0 == _0);
    assert(_1 == _1);
    assert(xx == xx);

    assert(xx < _0);
    assert(_0 < _1);
    assert(xx < _1);
  }
}

/// The next three tests illustrate:
///  1. How to use representation_of
///  2. How to use markable_dual_storage_type_unsafe
///  3. How to use std::pair as representaiton in dual storage.

class Date
{
  int _d;

public:
  bool invariant() const { return _d >= 0 && _d <= 100000; }
  explicit Date (int d) : _d(d) { assert(invariant()); }
  int as_int() const { return _d; }
  friend bool operator< (Date const& l, Date const& r) { return l._d <  r._d; }
  friend bool operator==(Date const& l, Date const& r) { return l._d == r._d; }
};

int DateInterval_value_ctor_calls_count = 0;
int DateInterval_copy_ctor_calls_count = 0;
int DateInterval_move_ctor_calls_count = 0;
int DateInterval_dtor_calls_count = 0;

class DateInterval
{
  Date _first;
  Date _last;

public:
  bool invariant() const { return !(_last < _first); }
  explicit DateInterval(Date const& f, Date const& l) : _first(f), _last(l) { assert (invariant()); ++DateInterval_value_ctor_calls_count; }
  DateInterval(const DateInterval& r) : _first(r._first), _last(r._last) { assert (invariant()); ++DateInterval_copy_ctor_calls_count; }
  DateInterval(DateInterval&& r) : _first(r._first), _last(r._last) { assert (invariant()); ++DateInterval_move_ctor_calls_count; }
  ~DateInterval() { assert (invariant()); ++DateInterval_dtor_calls_count; }

  DateInterval& operator=(const DateInterval& r) { assert (invariant()); _first = r._first; _last = r._last; assert (invariant()); return *this; }
  DateInterval& operator=(DateInterval&& r) { assert (invariant()); _first = r._first; _last = r._last; assert (invariant()); return *this; }

  Date const& first() const { return _first; }
  Date const& last() const { return _last; }

  friend bool operator==(DateInterval const& l, DateInterval const& r) { return l.first() == r.first() && l.last() == r.last(); }
};

namespace ak_toolkit { namespace markable_ns {
  template<> struct representation_of<DateInterval>
  {
    typedef std::pair<Date, Date> type;
  };
}}

void reset_Interval_count ()
{
  DateInterval_value_ctor_calls_count = 0;
  DateInterval_copy_ctor_calls_count = 0;
  DateInterval_move_ctor_calls_count = 0;
  DateInterval_dtor_calls_count = 0;
}

bool count_Interval_value_copy_move_dtror(int v, int c, int m, int d)
{
  return DateInterval_value_ctor_calls_count == v
      && DateInterval_copy_ctor_calls_count == c
      && DateInterval_move_ctor_calls_count == m
      && DateInterval_dtor_calls_count == d;
};

struct mark_interval : markable_dual_storage_type_unsafe<mark_interval, DateInterval /*, std::pair<Date, Date>*/ >
{
  static representation_type marked_value() { return std::make_pair(Date{2}, Date{1}); }
  static bool is_marked_value(const representation_type& v) { return v.second < v.first; }
};

void test_dual_storage_with_tuple_default_and_move_ctor()
{
  reset_Interval_count();
  {
    typedef markable<mark_interval> opt_interval;
    opt_interval o1;
    assert (!o1.has_value());
    assert (count_Interval_value_copy_move_dtror(0, 0, 0, 0));

    opt_interval o2 {DateInterval{Date{1}, Date{2}}};
    assert (o2.has_value());
    assert (o2.value().first() == Date{1});
    assert (o2.value().last()  == Date{2});
    assert (count_Interval_value_copy_move_dtror(1, 0, 1, 1));
  }
  assert (count_Interval_value_copy_move_dtror(1, 0, 1, 2));
}

void test_dual_storage_with_tuple_copy_ctor()
{
  reset_Interval_count();
  {
    typedef markable<mark_interval> opt_interval;
    DateInterval i12 {Date{1}, Date{2}};
    assert (count_Interval_value_copy_move_dtror(1, 0, 0, 0));

    opt_interval o2 {i12};
    assert (o2.has_value());
    assert (o2.value() == i12);
    assert (count_Interval_value_copy_move_dtror(1, 1, 0, 0));
  }
  assert (count_Interval_value_copy_move_dtror(1, 1, 0, 2));
}

void test_dual_storage_with_tuple_init_state_mutation()
{
  reset_Interval_count();
  {
    typedef markable<mark_interval> opt_interval;
    DateInterval i12 {Date{1}, Date{2}};
    assert (count_Interval_value_copy_move_dtror(1, 0, 0, 0));

    opt_interval o1 {}, o2 {i12};
    assert (count_Interval_value_copy_move_dtror(1, 1, 0, 0));

    assert (!o1.has_value());
    assert (o2.has_value());

    o1 = opt_interval{i12};
    assert (count_Interval_value_copy_move_dtror(1, 2, 1, 1));
    o2 = {};
    assert (count_Interval_value_copy_move_dtror(1, 2, 1, 2));

    assert (o1.has_value());
    assert (o1.value() == i12);
    assert (!o2.has_value());

    swap (o1, o2);
    assert (count_Interval_value_copy_move_dtror(1, 2, 2, 3));

    assert (!o1.has_value());
    assert (o2.has_value());
    assert (o2.value() == i12);
  }
  assert (count_Interval_value_copy_move_dtror(1, 2, 2, 5));
}

namespace most_hostile_types {

  template <typename Val, typename Ref, typename Rep, typename Store>
  struct mark_hostile_for
  {
    using value_type = Val;
    using reference_type = Ref;
    using representation_type = Rep;
    using storage_type = Store;

    static representation_type marked_value() AK_TOOLKIT_NOEXCEPT { return {0}; }
    static bool is_marked_value(const representation_type& v) { return v.v == -1; }

    static reference_type access_value(const Store& s) { return Ref{&s.val.v}; }
    static const representation_type& representation(const Store& s) { return s.rep; }
    static storage_type store_value(const value_type& v) { return Store{v.v, 0}; }
    static storage_type store_value(value_type&& v) { return Store{v.v, 0}; }
    static storage_type store_representation(const representation_type& v) { return Store{0, v.v}; }
    static storage_type store_representation(representation_type&& v) { return Store{0, v.v}; }
  };

  template <typename MP, typename OP>
  void test_interface_for()
  {
    using Rep = typename markable<MP, OP>::representation_type;
    using Val = typename markable<MP, OP>::value_type;

    markable<MP, OP> mh1, mh2{Val{1}};
    const Rep rep{2};
    const Val val{2};
    mh1 = mh2;
    mh1.assign(Val{2});
    mh1.assign(val);
    (void)mh1.has_value();
    (void)mh1.value();
    (void)mh1.representation_value();

    mh2.assign_representation(Rep{2});
    mh2.assign_representation(rep);

    swap(mh1, mh2);
  }

  template <typename MP, typename OP>
  void test_interface_with_relops_for()
  {
    using Val = typename markable<MP, OP>::value_type;

    test_interface_for<MP, OP>();
    markable<MP, OP> mh1, mh2{Val{1}};
    bool b = false;
    b = (mh1 < mh2);
    b = (mh1 > mh2);
    b = (mh1 <= mh2);
    b = (mh1 >= mh2);
    b = (mh1 == mh2);
    b = (mh1 != mh2);
    (void)b;
  }

  struct order_custom
  {
    template <typename M>
    static bool equal(const M&, const M&) { return true; }

    template <typename M>
    static bool less(const M&, const M&) { return false; }
  };

  namespace with_order_none
  {
    struct Val { int v; };
    struct Ref { int const* v; };
    struct Rep { int v; };
    struct Store { Val val; Rep rep; };

    using mark_hostile = mark_hostile_for<Val, Ref, Rep, Store>;

    void test()
    {
      test_interface_for<mark_hostile, order_none>();
      test_interface_with_relops_for<mark_hostile, order_custom>();
      test_interface_for<mark_hostile, order_by_value>();
      test_interface_for<mark_hostile, order_by_representation>();
    }
  }

  namespace with_order_by_value
  {
    struct Val { int v; };
    struct Ref { int const* v; };
    struct Rep { int v; };
    struct Store { Val val; Rep rep; };

    bool operator==(Ref, Ref) { return true; }
    bool operator<(Ref, Ref) { return false; }

    using mark_hostile = mark_hostile_for<Val, Ref, Rep, Store>;

    void test()
    {
      test_interface_for<mark_hostile, order_none>();
      test_interface_with_relops_for<mark_hostile, order_custom>();
      test_interface_with_relops_for<mark_hostile, order_by_value>();
      test_interface_for<mark_hostile, order_by_representation>();
    }
  }

  namespace with_order_by_representation
  {
    struct Val { int v; };
    struct Ref { int const* v; };
    struct Rep { int v; };
    struct Store { Val val; Rep rep; };

    bool operator==(Rep, Rep) { return true; }
    bool operator<(Rep, Rep) { return false; }

    using mark_hostile = mark_hostile_for<Val, Ref, Rep, Store>;

    void test()
    {
      test_interface_for<mark_hostile, order_none>();
      test_interface_with_relops_for<mark_hostile, order_custom>();
      test_interface_for<mark_hostile, order_by_value>();
      test_interface_with_relops_for<mark_hostile, order_by_representation>();
    }
  }



# if defined AK_TOOLKIT_WITH_CONCEPTS
static_assert(mark_policy<with_order_none::mark_hostile>, "mark_policy test failed");
static_assert(mark_policy<with_order_by_value::mark_hostile>, "mark_policy test failed");
static_assert(mark_policy<test_interface_with_relops_for::mark_hostile>, "mark_policy test failed");
# endif


  void test()
  {
    with_order_none::test();
    with_order_by_value::test();
    with_order_by_representation::test();
  }
}

namespace nested_markable
{
  struct mark_markable_bool : markable_type<markable<mark_bool>, char, markable<mark_bool>>
  {
    static representation_type marked_value() { return char(4); }
    static bool is_marked_value(const representation_type& v) { return v == 4; }

    static value_type access_value(const storage_type& v) { return value_type(with_representation, v); }
    static storage_type store_value(const value_type& v) { return v.representation_value(); }
  };

  using optopt_bool = markable<mark_markable_bool>;

  void test_opt_opt_bool()
  {
    static_assert(sizeof(optopt_bool) == sizeof(char), "excessive size");
    optopt_bool oob {};
    assert(!oob.has_value());
    assert(char(4) == oob.representation_value());

    markable<mark_bool> ob {};
    assert(char(2) == ob.representation_value());
    assert(!ob.has_value());

    oob.assign(ob);
    assert(oob.has_value());
    assert(!oob.value().has_value());
    assert(char(2) == oob.representation_value());

    oob.assign(markable<mark_bool>{true});
    assert(oob.has_value());
    assert(oob.value().has_value());
    assert(oob.value().value() == true);
    assert(char(1) == oob.representation_value());
  }


  using opt_int = markable<mark_int<int, -1>>;
  struct mark_minus_2 : markable_type<opt_int, int, opt_int>
  {
    static representation_type marked_value() { return -2; }
    static bool is_marked_value(const representation_type& v) { return v == -2; }

    static value_type access_value(const storage_type& v) { return value_type(with_representation, v); }
    static storage_type store_value(const value_type& v) { return v.representation_value(); }
  };

  using optopt_int = markable<mark_minus_2>;

  void test_opt_opt_int()
  {
    static_assert(sizeof(optopt_int) == sizeof(int), "excessive size");
    optopt_int ooi {};
    assert(!ooi.has_value());
    assert(-2 == ooi.representation_value());


    opt_int oi {};
    assert(-1 == oi.representation_value());
    assert(!oi.has_value());

    ooi.assign(oi);
    assert(ooi.has_value());
    assert(!ooi.value().has_value());
    assert(-1 == ooi.representation_value());

    ooi.assign(opt_int{1});
    assert(ooi.has_value());
    assert(ooi.value().has_value());
    assert(ooi.value().value() == true);
    assert(1 == ooi.representation_value());
  }

  void test()
  {
    test_opt_opt_bool();
    test_opt_opt_int();
  }
}

int main()
{
  test_value_ctor();
  test_assignment();
  test_string_traits();
  test_custom_storage();
  test_bool_storage();
  test_representation_value();
  test_mark_fp_nan();
  test_mark_value_init();
  test_mark_stl_empty();
  test_mark_enum();
  test_true_or_novalue();
  test_order_by_representation();
  test_order_by_value();

#if defined AK_TOOLKIT_USING_BOOST
  test_optional_as_storage();
#endif

  test_mark_dual_storage_1();
  test_mark_dual_storage_2();
  test_dual_storage_ordering();
  test_default_markable();
  test_dual_storage_with_tuple_default_and_move_ctor();
  test_dual_storage_with_tuple_copy_ctor();
  test_dual_storage_with_tuple_init_state_mutation();
  most_hostile_types::test();
  nested_markable::test();
  test_manual_cmp();
}
