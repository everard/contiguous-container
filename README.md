This repository contains implementation of the R1D4 version of [contiguous_container](https://everard.github.io/contiguous_container).
[Catch](https://github.com/philsquared/Catch/) is used for unit testing.

Header storage_types.h (WIP) implements some common storage types, which are used in definition of common container types in contiguous_container.h header file:
 - vector - normal vector, almost the same as std::vector.

TODO:
 - inplace_vector - satisfies sequence container requirements, uses embedded storage for N elements, capacity won't change over time;
 - small_vector - fully satisfies allocator-aware container requirements, uses embedded storage for N elements, and when
   capacity is exhausted, uses allocator to obtain more memory;
 - [Kevin Hall’s fixed_vector](https://github.com/KevinDHall/Embedded-Containers)
