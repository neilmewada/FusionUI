// Preprocessor utilities
// Macros with __ prefix are considered engine internals and should not be used directly

// Converts parameter to string, guarantees macro expansion 2 levels
// Ex: FUSION_TOSTRING(arg) <=> "arg"
#define FUSION_TOSTRING(arg)  __FUSION_TOSTRING1(arg)
#define __FUSION_TOSTRING1(arg) __FUSION_TOSTRING2(arg)
#define __FUSION_TOSTRING2(arg) #arg

// Concatenates macro parameters text to string, guarantees macro expansion 2 levels
#define FUSION_CONCATENATE(arg1, arg2)   __FUSION_CONCATENATE1(arg1, arg2)
#define __FUSION_CONCATENATE1(arg1, arg2)  __FUSION_CONCATENATE2(arg1, arg2)
#define __FUSION_CONCATENATE2(arg1, arg2)  arg1 ## arg2

//Used most notably for MSVC's different behavior in handling __VA_ARGS__.
//In MSVC __VA_ARGS__ is expanded as a single token which means the next macro will only have one parameter
//To work around this replace MACRO(__VA_ARGS__) by FUSION_MACRO_EXPAND(MACRO(__VA_ARGS)) which will perform correct substitution
//More background info in https://stackoverflow.com/questions/5134523/msvc-doesnt-expand-va-args-correctly
#define FUSION_MACRO_EXPAND(va_args) va_args

#define FUSION_EXPAND(va_args) va_args

#define __FUSION_FIRST_ARG(First, ...) First
#define FUSION_FIRST_ARG(...) FUSION_MACRO_EXPAND(__FUSION_FIRST_ARG(__VA_ARGS__))

#define __FUSION_NON_FIRST_ARGS(First, ...) __VA_ARGS__
#define FUSION_NON_FIRST_ARGS(...) FUSION_MACRO_EXPAND(__FUSION_NON_FIRST_ARGS(__VA_ARGS__))

#ifdef _MSC_VER // Microsoft compilers

#   define FUSION_ARG_COUNT(...)  __FUSION_EXPAND_ARGS_PRIVATE(__FUSION_ARGS_AUGMENTER(__VA_ARGS__))

#   define __FUSION_ARGS_AUGMENTER(...) unused, __VA_ARGS__
#   define __FUSION_EXPAND(x) x
#   define __FUSION_EXPAND_ARGS_PRIVATE(...) __FUSION_EXPAND(__FUSION_GET_ARG_COUNT_PRIVATE(__VA_ARGS__, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#   define __FUSION_GET_ARG_COUNT_PRIVATE(_1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, _17_, _18_, _19_, _20_, _21_, _22_, _23_, _24_, _25_, _26_, _27_, _28_, _29_, _30_, _31_, _32_, _33_, _34_, _35_, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, count, ...) count

#else // Non-Microsoft compilers

#   define FUSION_ARG_COUNT(...) __FUSION_GET_ARG_COUNT_PRIVATE(0, ## __VA_ARGS__, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#   define __FUSION_GET_ARG_COUNT_PRIVATE(_0, _1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, _17_, _18_, _19_, _20_, _21_, _22_, _23_, _24_, _25_, _26_, _27_, _28_, _29_, _30_, _31_, _32_, _33_, _34_, _35_, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, count, ...) count

#endif


#define FUSION_ENUM_CLASS(Enum)\
	inline			 bool  operator==(Enum E, i32 Rhs) { return E == (Enum)Rhs; }\
	inline			 bool  operator!=(Enum E, i32 Rhs) { return E != (Enum)Rhs; }\
    inline           bool  operator==(Enum E, i64 Rhs) { return E == (Enum)Rhs; }\
    inline           bool  operator!=(Enum E, i64 Rhs) { return E != (Enum)Rhs; }

#define FUSION_ENUM_CLASS_FLAGS(Enum) \
	inline           Enum& operator|=(Enum& Lhs, Enum Rhs) { return Lhs = (Enum)((__underlying_type(Enum))Lhs | (__underlying_type(Enum))Rhs); } \
	inline           Enum& operator&=(Enum& Lhs, Enum Rhs) { return Lhs = (Enum)((__underlying_type(Enum))Lhs & (__underlying_type(Enum))Rhs); } \
	inline           Enum& operator^=(Enum& Lhs, Enum Rhs) { return Lhs = (Enum)((__underlying_type(Enum))Lhs ^ (__underlying_type(Enum))Rhs); } \
	inline constexpr Enum  operator| (Enum  Lhs, Enum Rhs) { return (Enum)((__underlying_type(Enum))Lhs | (__underlying_type(Enum))Rhs); } \
	inline constexpr Enum  operator& (Enum  Lhs, Enum Rhs) { return (Enum)((__underlying_type(Enum))Lhs & (__underlying_type(Enum))Rhs); } \
	inline constexpr Enum  operator^ (Enum  Lhs, Enum Rhs) { return (Enum)((__underlying_type(Enum))Lhs ^ (__underlying_type(Enum))Rhs); } \
	inline constexpr bool  operator! (Enum  E)             { return !(__underlying_type(Enum))E; } \
	inline constexpr Enum  operator~ (Enum  E)             { return (Enum)~(__underlying_type(Enum))E; }\
	inline			 bool  operator==(Enum E, i32 Rhs) { return E == (Enum)Rhs; }\
	inline			 bool  operator!=(Enum E, i32 Rhs) { return E != (Enum)Rhs; }\
    inline           bool  operator==(Enum E, i64 Rhs) { return E == (Enum)Rhs; }\
    inline           bool  operator!=(Enum E, i64 Rhs) { return E != (Enum)Rhs; }\
	inline constexpr u32  operator& (Enum  Lhs, u32 Rhs) { return (u32)((__underlying_type(Enum))Lhs & (__underlying_type(Enum))Rhs); }\
	inline constexpr u32  operator| (Enum  Lhs, u32 Rhs) { return (u32)((__underlying_type(Enum))Lhs | (__underlying_type(Enum))Rhs); }

// Applies macro M to each argument (up to 64). Internal helpers use __ prefix.
#define __FUSION_FOR_EACH_1( M, a)      M(a)
#define __FUSION_FOR_EACH_2( M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_1( M, __VA_ARGS__))
#define __FUSION_FOR_EACH_3( M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_2( M, __VA_ARGS__))
#define __FUSION_FOR_EACH_4( M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_3( M, __VA_ARGS__))
#define __FUSION_FOR_EACH_5( M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_4( M, __VA_ARGS__))
#define __FUSION_FOR_EACH_6( M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_5( M, __VA_ARGS__))
#define __FUSION_FOR_EACH_7( M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_6( M, __VA_ARGS__))
#define __FUSION_FOR_EACH_8( M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_7( M, __VA_ARGS__))
#define __FUSION_FOR_EACH_9( M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_8( M, __VA_ARGS__))
#define __FUSION_FOR_EACH_10(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_9( M, __VA_ARGS__))
#define __FUSION_FOR_EACH_11(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_10(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_12(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_11(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_13(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_12(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_14(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_13(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_15(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_14(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_16(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_15(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_17(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_16(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_18(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_17(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_19(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_18(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_20(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_19(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_21(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_20(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_22(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_21(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_23(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_22(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_24(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_23(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_25(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_24(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_26(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_25(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_27(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_26(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_28(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_27(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_29(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_28(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_30(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_29(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_31(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_30(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_32(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_31(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_33(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_32(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_34(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_33(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_35(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_34(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_36(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_35(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_37(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_36(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_38(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_37(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_39(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_38(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_40(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_39(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_41(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_40(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_42(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_41(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_43(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_42(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_44(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_43(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_45(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_44(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_46(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_45(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_47(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_46(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_48(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_47(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_49(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_48(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_50(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_49(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_51(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_50(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_52(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_51(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_53(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_52(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_54(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_53(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_55(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_54(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_56(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_55(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_57(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_56(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_58(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_57(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_59(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_58(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_60(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_59(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_61(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_60(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_62(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_61(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_63(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_62(M, __VA_ARGS__))
#define __FUSION_FOR_EACH_64(M, a, ...) M(a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_63(M, __VA_ARGS__))

#define FUSION_FOR_EACH(M, ...) \
    FUSION_MACRO_EXPAND(FUSION_CONCATENATE(__FUSION_FOR_EACH_, FUSION_ARG_COUNT(__VA_ARGS__))(M, __VA_ARGS__))

// FUSION_FOR_EACH_CTX — like FUSION_FOR_EACH but passes a fixed CTX as first arg: M(CTX, elem).
#define __FUSION_FOR_EACH_CTX_1( M, CTX, a)      M(CTX, a)
#define __FUSION_FOR_EACH_CTX_2( M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_1( M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_3( M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_2( M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_4( M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_3( M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_5( M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_4( M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_6( M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_5( M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_7( M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_6( M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_8( M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_7( M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_9( M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_8( M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_10(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_9( M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_11(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_10(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_12(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_11(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_13(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_12(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_14(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_13(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_15(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_14(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_16(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_15(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_17(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_16(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_18(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_17(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_19(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_18(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_20(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_19(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_21(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_20(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_22(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_21(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_23(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_22(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_24(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_23(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_25(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_24(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_26(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_25(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_27(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_26(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_28(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_27(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_29(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_28(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_30(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_29(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_31(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_30(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_32(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_31(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_33(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_32(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_34(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_33(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_35(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_34(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_36(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_35(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_37(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_36(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_38(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_37(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_39(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_38(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_40(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_39(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_41(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_40(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_42(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_41(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_43(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_42(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_44(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_43(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_45(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_44(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_46(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_45(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_47(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_46(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_48(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_47(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_49(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_48(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_50(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_49(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_51(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_50(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_52(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_51(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_53(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_52(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_54(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_53(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_55(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_54(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_56(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_55(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_57(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_56(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_58(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_57(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_59(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_58(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_60(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_59(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_61(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_60(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_62(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_61(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_63(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_62(M, CTX, __VA_ARGS__))
#define __FUSION_FOR_EACH_CTX_64(M, CTX, a, ...) M(CTX, a) FUSION_MACRO_EXPAND(__FUSION_FOR_EACH_CTX_63(M, CTX, __VA_ARGS__))

#define FUSION_FOR_EACH_CTX(M, CTX, ...) \
    FUSION_MACRO_EXPAND(FUSION_CONCATENATE(__FUSION_FOR_EACH_CTX_, FUSION_ARG_COUNT(__VA_ARGS__))(M, CTX, __VA_ARGS__))

// Folds variadic arguments with | (up to 8).
#define __FUSION_FOLD_OR_1(a)                a
#define __FUSION_FOLD_OR_2(a, b)             a | b
#define __FUSION_FOLD_OR_3(a, b, c)          a | b | c
#define __FUSION_FOLD_OR_4(a, b, c, d)       a | b | c | d
#define __FUSION_FOLD_OR_5(a, b, c, d, e)    a | b | c | d | e
#define __FUSION_FOLD_OR_6(a, b, c, d, e, f) a | b | c | d | e | f

#define FUSION_FOLD_OR(...) \
    FUSION_MACRO_EXPAND(FUSION_CONCATENATE(__FUSION_FOLD_OR_, FUSION_ARG_COUNT(__VA_ARGS__))(__VA_ARGS__))

#define FUSION_BIT(x) (1 << (x))
#define FUSION_COUNT(arr) ::Fusion::FCountOf(&arr)

namespace Fusion
{

	template<typename Enum>
	constexpr bool FEnumHasAllFlags(Enum Flags, Enum Contains)
	{
		return (((__underlying_type(Enum))Flags) & (__underlying_type(Enum))Contains) == ((__underlying_type(Enum))Contains);
	}

	template<typename Enum>
	constexpr bool FEnumHasFlag(Enum Flags, Enum Contains)
	{
		return ((((__underlying_type(Enum))Flags) & (__underlying_type(Enum))Contains) != 0) || ((__underlying_type(Enum))Contains == 0);
	}

	template<typename T, size_t TSize>
	constexpr size_t FCountOf([[maybe_unused]] T (*arr)[TSize])
	{
		return TSize;
	}

}
