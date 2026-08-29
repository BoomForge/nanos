#ifndef KERNEL_COMPILER_H
#define KERNEL_COMPILER_H

#if defined(__GNUC__)
#define PACKED __attribute__((packed))
#define ALIGNED(n) __attribute__((aligned(n)))
#define NORETURN __attribute__((noreturn))
#else
#define PACKED
#define ALIGNED(n)
#define NORETURN
#endif

#endif
