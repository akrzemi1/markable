How do I do X with this library
===============================

How do I marshal and unmarshal markable `int` types through a framework that only understands `int`?
----------------------------------------------------------------------------------------------------

Such framework doesn't have to know that -1 is a special value, so
use the `representation` interface:

```c++
markable<mark_int<int, -1>> oi = /* ... */;
lib::write_int(ot.representation_value()); // from markable
oi.assign_representation(lib::read_int()); // to markable
```


How do I model "true or no-value"?
----------------------------------

This is often needed to represent "flags" that turn on optional features. You can use `mark_int`:

```c++
using flag = markable<mark_int<bool, false>>;
flag off_{}, false_{false}, true_{true};

assert (!off_.has_value());
assert (!false_.has_value());
assert (true_.has_value());
```

Later, when you need to pass this value to an interface that understands `bool`s but no optional values, you can use:

```c++
bool convert_to_bool(flag f) {
  return f.representation_value();
}
```  

Or when you need to convert it to `boost::opitonal<bool>` where value of `false` is never used:

```c++
boost::optional<bool> convert_to_opt_bool(flag f) {
  return boost::make_optional(f, true);
}
```
