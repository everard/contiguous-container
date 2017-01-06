This repository contains implementation of the R1D4 version of [contiguous_container](https://everard.github.io/contiguous_container).
[Catch](https://github.com/philsquared/Catch/) is used for unit testing.

Header containers.h (WIP) implements some common container types using contiguous_container:
 - vector - normal vector, almost the same as std::vector.

TODO:
 - inplace_vector - satisfies sequence container requirements, uses embedded storage for N elements, capacity won't change over time;
 - small_vector - fully satisfies allocator-aware container requirements, uses embedded storage for N elements, and when
   capacity is exhausted, uses allocator to obtain more memory;
 - [Kevin Hall’s fixed_vector](https://github.com/KevinDHall/Embedded-Containers)
