
# Dynamic crc32 hash, meet static crc32 hash

The classic pattern for parsing string tokens for values from a small set of choices goes something like this:

```cpp
std::string tok = ...;

if (tok == "abcd")
{
    // ...
}
else if (tok == "fgh")
{
    // ...
}
else if (tok == "abracadabra")
{
    // ...
```

This uses string comparisons (each of which is linear in the string length) and a number of such comparison trials
is also linear in the number of possible tokens (on average). We would like to replace such code with something more
`switch`-like:

```cpp
std::string tok = ...;

switch (hash(tok))
{
    case hash("abcd"):
    {
        // ...
    }
    break;
    
    case hash("fgh"):
    {
        // ...
    }
    break;
    
    case hash("abracadabra"):
    {
        // ...
```

Not only does this look more maintainable, there is a good chance we can pick up some performance if we end up saving
on long string comparisons. (Note that if `hash()` produces duplicates the switch won't compile, so we're safe against
false negatives <sup name="a1">[1](#f1)</sup>. And since `hash("abcd")` doesn't change at runtime we should be able to save even more by pre-computing it.

Syntactically, it is possible to engineer a single `hash()` function that will work both for runtime inputs like `tok`
and for the `case` values that must be *compile-time constants* in C++. However, I hope to make it clear below
that in our particular use case this approach is neither optimal nor particularly convenient.

Let's now work through a practical solution in steps.

## A slow hash is "fast enough" when done at compile time

First, we start with a "slow" reference implementation of CRC32C as used by iSCSI protocol:

```cpp
uint32_t const g_crctable [256] =
{
    0x00000000L, 0xF26B8303L, 0xE13B70F7L, 0x1350F3F4L,
    0xC79A971FL, 0x35F1141CL, 0x26A1E7E8L, 0xD4CA64EBL,
    ...
};

uint32_t
crc32_reference (uint8_t const * buf, int32_t len, uint32_t crc)
{
    while (len --)
    {
        crc = (crc >> 8) ^ g_crctable [(crc ^ (* buf ++)) & 0xFF];
    }

    return crc;
}    
```
(this is *almost* the version used by iSCSI: the code chooses not to flip the bits of `crc` before returning that value)

---
Why use a CRC as a hash and, more interestingly, why use the iSCSI version of it? 

 1. because CRCs have mathematical properties that guarantee, with high probability, different values for runs of data
    differing even in a single bit <sup name="a2">[2](#f2)</sup> -- this is one of the properties we'd want in a good hash function;
    
 2. because this particular CRC version is supported directly by hardware via the `crc32c` [SSE 4.2 instruction](https://en.wikipedia.org/wiki/X86_instruction_listings#Added_with_SSE4.2) for use, unsurprisingly, with iSCSI devices -- and that fact will be important to our faster runtime version;
    
 3. and last, but not least, the reference implementation looks simple enough to be easily convertible to a `constexpr`
    version even with the C++11 restricted support for `constexpr`s.  
 
I called the above version "slow" because just about the only thing that makes it faster than reading data one bit at
a time is reading data one *byte* at a time, as it does. (A competitive runtime version would leverage SIMD parallelism,
consume data in multi-byte chunks, minimize branches, etc.)

---
However, this version is also simple enough to get correct easily and implementations very similar to the above can be
found in many an open source project, Linux module sources, etc. It will be a useful "known good" implementation for
testing any optimized variants.

This simplicity is also what makes it great for converting to a `constexpr`:

 - a fixed-size array of compile-time constants becomes `constexpr` simply by adding a specifier;
 
 - the `while` loop advancing over `buf` and accumulating in `crc` easily becomes a recursive `constexpr` function
   (recursions and ?:-operator are staple "looping" techniques in C++11 `constexpr`s):

```cpp
constexpr uint32_t
crc32_constexpr (char const * str, int32_t len, uint32_t crc, int32_t pos = 0)
{
    return (pos == len ? crc : crc32_constexpr (str, len, (crc >> 8) ^ g_crctable [(crc ^ str [pos]) & 0xFF], pos + 1));
}
```
So, now expressions like `crc32_constexpr("hello", 5, -1)` will accumulate 5 bytes of `"hello"` into an iSCSI CRC32 value starting from seed `-1` -- and *do that at compile-time*. (You could use any non-zero seed value.) 

Obviously, counting chars in a string literal to arrive at 5 like that isn't terribly convenient or error-proof. We
can't just use `std::strlen()` either because that would ruin our `constexpr`-ness. Fortunately, another C++11
feature solves the problem neatly. In fact, it is scarily neat.

## My kind of syntactic sugar: user-defined literals

A less well-known enhancement to the language brought about by C++11 standard is [*user-defined literals*](http://en.cppreference.com/w/cpp/language/user_literal).
They can be used to define all sorts of nifty suffixes on numeric and string literals via custom operators of the
form `operator "" _SOMETHING` (invoked in response to tokens like `10_SOMETHING`) *and* they are allowed to
be `constexpr`. One of the allowed signatures for a *string* literal operator is

```
( const char *, std::size_t )
```
where the compiler will provide the `std::strlen()`-like string length, i.e. excluding the null terminator.

Long story short, if we define

```
constexpr uint32_t
operator "" _hash (char const * str, std::size_t len)
{
    return crc32_constexpr (str, len, -1);
}
```
we will be able to say simply `"hello"_hash` and that expression will evaluate to our CRC hash value at both compile
and run times. Not too shabby.

## A fast(er) dynamic hash via SSE

As mentioned above, CRC variant used by iSCSI is supported directly by modern CPUs via the `crc32c` instruction. It can
consume data in chunks of up to 8 bytes, is reasonably fast, and pipelines well <sup name="a3">[3](#f3)</sup>. Something like the following
would be dramatically faster than any hand-tuned version of `crc32_reference()`:

```cpp
inline uint32_t
crc32 (uint8_t const * buf, int32_t len, uint32_t crc)
{
    while (VR_UNLIKELY (len >= 8)) // more perf tweaking can be done here, but at least tell the compiler we expect short strings
    {
        crc = i_crc32 (crc, * reinterpret_cast<uint64_t const *> (buf));
        buf += 8; len -= 8;
    }

    switch (len) // consume remaining bytes:
    {
        case 7:
            crc = i_crc32 (crc, * buf); ++ buf;
            /* no break */
        case 6:
            crc = i_crc32 (crc, * reinterpret_cast<uint32_t const *> (buf));
            crc = i_crc32 (crc, * reinterpret_cast<uint16_t const *> (buf + 4));
            return crc;
            
        // ...handle other remainder sizes... 
    }
}
```

The `i_crc32()` calls you see above are simple overloaded wrappers around `_mm_crc32_u{8,16,32,64}()` SSE intrinsics that make them more C++- and metaprogramming-friendly.

---
Computing hash functions with a `crc32c` assist has recently become popular so you should be able to find other, faster, variations
online. My implementation consumes every single byte of data, to match `crc32_constexpr()` output. Strictly speaking,
that isn't necessary for string hashing/lookup as needed by my use case: you could undersample long strings, for example,
or use prefixes/suffixes of fixed max lengths, etc to gain more performance. Such improvements tend to be application-
and data distribution-specific, however. My version is mildly hinted towards short strings to match my common use cases.  

## Almost there

All that remains is to add some overloaded wrappers for `crc32()` to accept strings in various forms that can be
encountered in the wild:

```cpp
inline uint32_t
str_hash (char const * str, int32_t len)
{
    return crc32 (reinterpret_cast<uint8_t const *> (str), len, -1);
}

/**
 * @param str [must be 0-terminated]
 */
inline uint32_t
str_hash (char const * str)
{
    return str_hash (str, std::strlen (str));
}

inline uint32_t
str_hash (std::string const & str)
{
    return str_hash (str.data (), str.size ());
}
```

and we can start `switch`ing on strings using the dynamic `str_hash()` in the condition and the static `"..."_hash`
values in the `case` statements:

```cpp
    switch (str_hash(av [1]))
    {
        case "abcd"_hash:           std::cout << "abcd"        << std::endl; break;
        case "fgh"_hash:            std::cout << "fgh"         << std::endl; break;
        case "abracadabra"_hash:    std::cout << "abracadabra" << std::endl; break;
    }
    
    std::cout << '\'' << av [1] << "' hashed to 0x" << std::hex << str_hash (av [1]) << std::endl;
```

Looks like a nice and tidy `switch` statement to me!

### build the demo

There are only 3 `.cpp` files in this demo repo. They are easy enough to build by hand if desired but you could also
try the included `build.sh` script:

```shell
>./build.sh
>../static_hash hello           # not a switch case
'hello' hashed to 0x658e44b3
>./static_hash fgh              # a case match
fgh
'fgh' hashed to 0x79e5f6b1
>./static_hash abracadabra      # another case match
abracadabra
'abracadabra' hashed to 0xd3c7a715
```
And run the test comparing the slow and fast CRC32C implementations:
 
```shell
>./static_hash test
running test_crc32() ..., success: 1
'test' hashed to 0x795f8d3f
```

## Recap: what have we gained, exactly?

A switch statement does not always guarantee O(1) dispatch cost: usually a compiler will use a jump table only if
the `case` values fall into compact groups and ranges. Otherwise it will have to emit a series of comparisons followed
by conditional jumps. In the latter case, the dispatch has a cost component that's linear in the number of `case`
statements, even at the assembly level.

Switching on hash values the way we do here will inevitably fall into the latter category (the whole point of a good
hash function is to "randomize" the output). Here is an asm extract from the `main.cpp`
demo above:

```asm
   0x0000000000400d15 <+85>:   je     0x400dea <main(int32_t, char**)+298>
   0x0000000000400d1b <+91>:    jbe    0x400e93 <main(int32_t, char**)+467>
   0x0000000000400d21 <+97>:    cmp    $0x79e5f6b1,%ebx
   0x0000000000400d27 <+103>:   je     0x400e43 <main(int32_t, char**)+387>
   0x0000000000400d2d <+109>:   cmp    $0xd3c7a715,%ebx
   0x0000000000400d33 <+115>:   jne    0x400d58 <main(int32_t, char**)+152>
```
If you squint, you should be able to see the hash values from the demo runs above used directly as `cmp` operands. The jumps that follow
them are the bodies of `case` statements. The upshot is:

 - we've replaced O(N) string comparisons with `int32_t` comparisons, with one side of the comparison prepared
   entirely at compile time;
 - these precomputed values are 32-bit literal asm operands (there is no jump table in the data segment and no
   corresponding memory load)

So, the next step: a `constexpr` *perfect* hash, anyone? :)

## Authors

[Vlad Roubtsov](https://github.com/vladium), 2017

## Acknowledgments

* I was inspired by a different, pre-C++11, solution for static string hashing in section 8.3 of Davide Di Gennaro's "Advanced C++ Metaprogramming".

---
<b id="f1">[1]</b>: Technically, false negatives are still possible: they are extremely unlikely (a 32-bit hash collision) and in this use case the tokens are coming from a finite set, they are not arbitrary input strings.[↩](#a1)

<b id="f2">[2]</b>: For example, a CRC based on a generator polynomial of degree *M* with a nonzero x<sup>0</sup> term will detect all possible bit changes in a consecuitive *M* bits of the input.[↩](#a2) 

<b id="f3">[3]</b>: Agner Fog's instruction tables [http://www.agner.org/optimize/instruction_tables.pdf] for Haswell list `crc32c` latency at only 3 cycles, with the CPU able to pipeline one instruction immediately after another.[↩](#a3)
