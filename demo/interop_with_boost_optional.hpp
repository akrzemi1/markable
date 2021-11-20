// Copyright (C) 2015 - 2021, Andrzej Krzemienski.
//
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// This demonstrates how you can convert from markable bool to boost::opitonal.
// Note: this doesn't work with std::opitonal.
// Requires the presence of Boost.Optional library in your system

#include "markable.hpp"
#include <boost/optional.hpp>

namespace at = ak_toolkit;
using flag = at::markable<at::mark_int<bool, false>>;

boost::optional<bool> convert_to_opt_bool(flag f)
{
  return boost::make_optional(f.has_value(), true);
}

int main()
{
  flag a, b{true};
  assert(boost::none == convert_to_opt_bool(a));
  assert(true == convert_to_opt_bool(b));
}
