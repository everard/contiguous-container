This repository contains implementation of the R1D1 version of [contiguous_container](https://everard.github.io/contiguous_container).

Header containers.h (work in progress) shall implement some common container types using contiguous_container:
 - (TODO) inplace_vector - satisfies sequence container requirements, uses embedded storage for N elements, capacity can't change over time;
 - (TODO) small_vector - fully satisfies allocator-aware container requirements, uses embedded storage for N elements, and when
    capacity is exhausted, uses allocator to obtain more memory;
 - vector (almost ready) - normal vector, almost the same as std::vector.
