# actf - A cute acute Common Trace Format (2) decoding library

actf is a C library for decoding a [Common Trace Format (CTF) Version
2][spec] metadata stream and data stream. The goal of actf is to be a
fast and spec-compliant CTF2 decoding library. It additionally
includes a command-line utility `actf` that can read and print a [CTF
2 trace stored on a file system][spec-fs].

actf is written for C11 and POSIX 2008. It has a single dependency,
[json-c][jsonc], to facilitate the parsing of the JSON fragment based
CTF2 metadata.

Since CTF2 is still in its infancy with regards to producers, this
library is mainly built based on the specification. Perhaps it works
or perhaps I have invented a decoder for a new custom binary format.

**actf does not support CTF version 1.X**

## Deviations from CTF 2 specification

### Fixed-length floating point number field class

The specification allows fixed-length floating point numbers to have
the following lengths (bits):
- 16
- 32
- 64
- 128
- *K*, where *K* is greater than 128 and a multiple of 32

In actf, only lengths 32 and 64 is supported.

### All fixed-length bit array derived classes

The specification does not put a limit on the number of bits a
fixed-length bit array can represent.

In actf, the maximum length of a fixed-length bit array field is
limited to 64.

This affects the following field classes:
- fixed-length bit array
- fixed-length bit map
- fixed-length boolean
- fixed-length floating point number

### Variable-length integer field class

The specification does not limit the size of a variable-length
integer.

In actf, a variable-length integer will be truncated to 64 bits.

### Range JSON integer limitations

A range consists of two JSON integers which represent the lower and
upper bound. JSON integers are not limited in size by a specification,
so the actual bounds differ per implementation.

In actf, the following applies:
- For integer field class mappings, the ranges will be represented by
  *int64_t* or *uint64_t* based on if the holding integer field class
  is signed or not. Thus a fixed-length-unsigned-integer's mapping
  ranges can represent 0 to UINT64\_MAX (inclusive), and a
  fixed-length-signed-integer's mapping ranges can represent
  INT64\_MIN to INT64\_MAX (inclusive).
  - A range out of bounds will be truncated or removed since an
    integer field wouldn't be able to represent it anyway.
- For selector-field-ranges of optional and variant field classes,
  each integer range set is required to be either fully representable
  as *uint64_t* or *int64_t*.
  - A range set containing one or more ranges with a negative JSON
    integer and a JSON integer larger than INT64\_MAX is not supported
    and will cause an error. Such a range set cannot be represented by
    the implementation and could cause a wrong decoding.

## Building actf

actf is built with cmake:

```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

The following options are supported by cmake:
| Variable    | Type | Default | Desc                                   |
|-------------|------|---------|----------------------------------------|
| BUILD_BIN   | BOOL | ON      | Whether to build the actf application  |
| BUILD_TESTS | BOOL | ON      | Whether to build unit tests            |
| BUILD_DOC   | BOOL | ON      | Whether to build doxygen documentation |

## Running tests

Unit tests are cunit-based and can be executed by building a test
executable (cmake option `BUILD_TESTS`) and then executing it. Here
shown using build-directory `build`:

```
$ ./build/tests.out
```

actf is heavily tested using functional tests which are diffed against
a reference-file. First make sure the test application is built (cmake
option `BUILD_BIN`). The functional tests can then be executed with
the `runtests.sh` script as follows (build-directory `build`):

```
$ ./runtests.sh -e ./build/actf
```

## Building documentation

The public API of actf is documented using doxygen. It is built by
enabling cmake option `BUILD_DOC` and then running make on the target
`doc`.

```
$ mkdir build
$ cd build
$ cmake -DBUILD_DOC=ON ..
$ make doc
$ firefox html/index.html
```

## Using libactf

Link to actf however you please and include the header `actf/actf.h`
in your code.

```c
#include <actf/actf.h>
```

See the [examples](./examples) folder for example applications and how
to build them.

## Multi-threading support

actf is single-threaded itself but uses no mutable static or global
variables, so as long as each thread has their own mutable structs, it
will be fine. E.g. two threads can run two separate actf\_decoder in
parallel using the same actf\_metadata.

## License

LGPLv3.0 or later, see COPYING and COPYING.LESSER.

[spec]: https://diamon.org/ctf/ "CTF2-SPEC-2.0"
[spec-fs]: https://diamon.org/ctf/files/CTF2-FS-1.0.html "CTF2-FS-1.0"
[jsonc]: https://github.com/json-c/json-c "json-c"
