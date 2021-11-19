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
