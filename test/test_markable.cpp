// Copyright (C) 2015 - 2021, Andrzej Krzemienski.
//
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// TODO: deleteme
#include <iostream>

#include "../include/ak_toolkit/markable.hpp"
#include <cassert>
#include <utility>
#include <string>



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
	  assert (oi_.storage_value() == -1);
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

void test_storage_value()
{
  typedef markable< mark_int<int, -1> > opt_int;
  opt_int oi_, oiN1(-1), oi0(0), oi1(1);

  assert ( oi_.storage_value() == -1);
  assert (oiN1.storage_value() == -1);
  assert ( oi0.storage_value() ==  0);
  assert ( oi1.storage_value() ==  1);
}

void test_mark_fp_nan()
{
  typedef markable< mark_fp_nan<double> > opt_double;
  opt_double o_, o1 (1.0), oNan (0.0/0.0);
  assert (!o_.has_value());
  assert ( o1.has_value());
  assert (!oNan.has_value());

  assert (o1.value() == 1.0);

  double v = o_.storage_value();
  assert (v != v);

  v = o1.storage_value();
  assert (v == 1.0);
  assert (v == v);

  v = oNan.storage_value();
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

    assert (o_.storage_value() == 0);
    assert (o1.storage_value() == 1);
    assert (oE.storage_value() == 0);
  }
  {
    typedef markable<mark_value_init<std::string>> opt_t;
    opt_t o_, o1 (std::string("one")), oE ((std::string()));

    assert (!o_.has_value());
    assert ( o1.has_value());
    assert (!oE.has_value());

    assert (o1.value() == "one");

    assert (o_.storage_value() == "");
    assert (o1.storage_value() == "one");
    assert (oE.storage_value() == "");
  }
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

    assert (o_.storage_value() == Cont(true));
    assert (o1.storage_value() == Cont(false));
    assert (oE.storage_value() == Cont(true));
  }
  {
    typedef markable<mark_stl_empty<std::string>> opt_t;
    opt_t o_, o1 (std::string("one")), oE ((std::string()));

    assert (!o_.has_value());
    assert ( o1.has_value());
    assert (!oE.has_value());

    assert (o1.value() == "one");

    assert (o_.storage_value() == "");
    assert (o1.storage_value() == "one");
    assert (oE.storage_value() == "");
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

  assert (o_.storage_value() == -1);
  assert (oN.storage_value() ==  0);
  assert (oW.storage_value() ==  3);
}


void test_assignment()
{
  typedef markable<mark_enum<Dir, -1>> opt_dir;
  opt_dir od;
  assert(!od.has_value());

  od.assign(Dir::W);
  assert(od.has_value());
  assert(od.value() == Dir::W);

  od.assign_storage(-1);
  assert(!od.has_value());

  od.assign_storage(0);
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

void test_comparison()
{
  {
  typedef markable<mark_int<int, 0>, cmp_by_storage> opt_int;
  opt_int n1 {-1}, x{}, p1 {+1};
  assert (n1 != x);
  assert (x != p1);
  assert (n1 != p1);
  assert (n1 == n1);
  assert (p1 == p1);
  assert (x == x);
  }
  {
    typedef markable<mark_negative, cmp_by_storage> opt_int;
    opt_int x{}, n1{-1}, _0 {0}, p1{+1};
    assert (x != n1);
    assert (n1 != x); // both marked, but different representation
    assert (n1 != _0);
    assert (_0 != p1);
    assert (p1 != _0);
    assert (x == x);
    assert (n1 == n1);
    assert (p1 == p1);
    assert (_0 == _0);
  }
  {
    typedef markable<mark_negative, cmp_by_value_eq> opt_int;
    opt_int x{}, n1{-1}, _0 {0}, p1{+1};
    assert (x == n1);
    assert (n1 == x); // both marked => equal (regardless of representation)
    assert (n1 != _0);
    assert (_0 != p1);
    assert (p1 != _0);
    assert (x == x);
    assert (n1 == n1);
    assert (p1 == p1);
    assert (_0 == _0);
  }
}

/*
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

struct RawDateInterval
{
  Date _first;
  Date _last;
};

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

struct mark_interval : markable_dual_storage_type< DateInterval, std::pair<Date, Date> >
{
  static storage_type marked_value() { return {Date{2}, Date{1}}; }
  static bool is_marked_value(const storage_type& v) { return v.second < v.first; }
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
*/





int main()
{
  test_value_ctor();
  test_assignment();
  test_string_traits();
  test_custom_storage();
  test_bool_storage();
  test_storage_value();
  test_mark_fp_nan();
  test_mark_value_init();
  test_mark_stl_empty();
  test_mark_enum();
  test_comparison();

#if defined AK_TOOLKIT_USING_BOOST
  test_optional_as_storage();
#endif

  test_mark_dual_storage_1();
  test_mark_dual_storage_2();
/*  test_dual_storage_with_tuple_default_and_move_ctor();
  test_dual_storage_with_tuple_copy_ctor();
  test_dual_storage_with_tuple_init_state_mutation();
*/
}
