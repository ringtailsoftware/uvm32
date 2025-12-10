#ifndef TARGET_STDINT_H
#define TARGET_STDINT_H 1

// <stdint>
typedef unsigned long long uint64_t;
typedef unsigned long uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef signed long long int64_t;
typedef signed long int32_t;
typedef signed short int16_t;
typedef signed char int8_t;

#ifndef size_assert
#define size_assert( what, howmuch ) \
  typedef char what##_size_wrong_[( !!(sizeof(what) == howmuch) )*2-1 ]
#endif

// sanity check
size_assert(uint64_t, 8);
size_assert(uint32_t, 4);
size_assert(uint16_t, 2);
size_assert(uint8_t, 1);
size_assert(int64_t, 8);
size_assert(int32_t, 4);
size_assert(int16_t, 2);
size_assert(int8_t, 1);

#define INT32_MIN 0

#endif
