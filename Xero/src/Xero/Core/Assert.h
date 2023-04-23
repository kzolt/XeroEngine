#pragma once

#ifdef XO_DEBUG
#define XO_ENABLE_ASSERTS
#endif

#ifdef XO_ENABLE_ASSERTS

#define XO_ASSERT_NO_MESSAGE(condition) { if(!(condition)) { XO_ERROR("Assertion Failed"); XO_DEBUGBREAK(); } }
#define XO_ASSERT_MESSAGE(condition, ...) { if(!(condition)) { XO_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); XO_DEBUGBREAK(); } }

#define XO_ASSERT_RESOLVE(arg1, arg2, macro, ...) macro
#define XO_GET_ASSERT_MACRO(...)  XO_EXPAND_VARGS(XO_ASSERT_RESOLVE(__VA_ARGS__, XO_ASSERT_MESSAGE, XO_ASSERT_NO_MESSAGE))

#define XO_ASSERT(...) XO_EXPAND_VARGS(XO_GET_ASSERT_MACRO(__VA_ARGS__)(__VA_ARGS__) )
#define XO_CORE_ASSERT(...) XO_EXPAND_VARGS(XO_GET_ASSERT_MACRO(__VA_ARGS__)(__VA_ARGS__) )

#else
#define XO_ASSERT(x, ...)
#define XO_CORE_ASSERT(x, ...)
#endif