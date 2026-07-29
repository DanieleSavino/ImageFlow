/**
 * @file constructor.h
 * @brief Portable "run before main" constructor macro.
 *
 * Wraps compiler-specific mechanisms for registering a function to run
 * automatically at load time, before main() executes:
 *  - GCC / Clang (and Clang-based ICX, and nvcc's host compiler): native
 *    __attribute__((constructor)) support.
 *  - MSVC: emulated via a CRT initializer section, since MSVC has no
 *    equivalent attribute.
 *
 * @warning Ordering between multiple IF_CONSTRUCTOR functions in different
 *          translation units is unspecified unless you assign explicit
 *          priorities (GCC/Clang only, see IF_CONSTRUCTOR_PRIO). Do not
 *          write a constructor that depends on another constructor having
 *          already run unless you pin both priorities explicitly.
 */
#pragma once

#if defined(__GNUC__) || defined(__clang__)

    /* GCC / Clang / Clang-based ICX / nvcc host-side. Priorities 0-100 are
     * reserved for the implementation; use 101+ if you need explicit ordering. */
    #define IF_CONSTRUCTOR(func) \
        static void func(void); \
        __attribute__((constructor)) \
        static void func(void)

    #define IF_CONSTRUCTOR_PRIO(func, prio) \
        static void func(void); \
        __attribute__((constructor(prio))) \
        static void func(void)

#elif defined(_MSC_VER)

    /* MSVC: no constructor attribute. Emulated by placing a pointer to the
     * function in the CRT's initializer section, which the runtime walks
     * before main(). No priority control here; MSVC decides ordering. */
    #pragma section(".CRT$XCU", read)

    #define IF_CONSTRUCTOR(func) \
        static void __cdecl func(void); \
        __declspec(allocate(".CRT$XCU")) \
        void (__cdecl *func##_)(void) = func; \
        static void __cdecl func(void)

    #define IF_CONSTRUCTOR_PRIO(func, prio) \
        IF_CONSTRUCTOR(func) /* priority not supported under MSVC; ignored */

#else
    #error "IF_CONSTRUCTOR: no supported mechanism for this compiler. " \
           "Add an explicit registration call from main() instead."
#endif
