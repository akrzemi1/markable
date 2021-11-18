# markable 2.0.0 [![Build Status](https://travis-ci.org/akrzemi1/markable.svg?branch=master)](https://travis-ci.org/akrzemi1/markable)

An alternative to `boost::optional<T>` which does not store an additional `bool` flag,
but encodes the 'empty' state inside `T` using a special indicated value.

(This library used to be called `compact_optional`, but it was renamed to avoid confusion with `std::experimental::optional`.)

## installation
It is a C++11 single-header (header-only) library.

## Usage

Do you want to store a possibly missing `int`? Can you spare value `-1`? You can use it like this:

```c++
using namespace ak_toolkit;
typedef markable<mark_int<int, -1>> opt_int;

opt_int oi;
opt_int o2 (2);

assert (!oi.has_value());
assert (o2.has_value());
assert (o2.value() == 2);

static_assert (sizeof(opt_int) == sizeof(int), "");
```

Do you want to store a possibly missing `std::string`, where 'missing' != 'empty'?
Can you spare some string values that contain a null character inside, like `std::string("\0\0", 2)`? This is how you do it:

```c++
struct string_marked_value                           // a policy which defines the representaioion of the
  : ak_toolkit::markable_type<std::string>           // 'marked' (special) std::string value
{               
  static std::string marked_value() {                // create the marked value
    return std::string("\0\0", 2);
  }
  static bool is_marked_value(const std::string& v) { // test if a given value is considered marked
    return v.compare(0, v.npos, "\0\0", 2) == 0;
  }
};

typedef ak_toolkit::markable<string_marked_value> opt_str;
opt_str os, oE(std::string(""));

assert (!os.has_value());
assert (oE.has_value());
assert (oE.value() == "");

static_assert (sizeof(opt_str) == sizeof(std::string), "");
```

Cannot spare any value, but still want to use this interface? You can use `boost::optional` or `std::experimental::optional` at the cost of storage size:

```c++
typedef markable<mark_optional<boost::optional<int>>> opt_int;
static_assert (sizeof(opt_int) == sizeof(boost::optional<int>), "");
```

We also provide a customization for optional `bool` with the size of a `char`:

```c++
typedef markable<mark_bool> opt_bool;
static_assert (sizeof(opt_bool) == 1, "");
```

For a more comprehensive overview of the library see [documentation/overview.adoc](documentation/overview.adoc).


For additional motivation and overview see this post: https://akrzemi1.wordpress.com/2015/07/15/efficient-optional-values/.


## Acknowledgments

The idea of encoding the policy for determining the marked state in a template parameter has been proposed by Vitali Lovich (see https://groups.google.com/a/isocpp.org/d/topic/std-proposals/46J1onhWJ-s/discussion).

Matt Calabrese and Tomasz Kami&#324;ski suggested the raw-storage representation for holding optional types with strong invariants.

Kyrylo Bohdanenko suggested the simple and elegant interface for ordering policies.


## License
Distributed under the [Boost Software License, Version 1.0](http://www.boost.org/LICENSE_1_0.txt).


***

_&copy; 2015 &mdash; 2021 Andrzej Krzemie&#324;ski_
