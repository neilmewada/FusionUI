#include <gtest/gtest.h>

#include "Fusion/Core.h"

#include <thread>
#include <vector>

using namespace Fusion;

#pragma region FString

TEST(FStringTest, DefaultConstructor)
{
    FString str;
    EXPECT_EQ(str.ByteLength(), 0);
    EXPECT_TRUE(str.Empty());
    EXPECT_STREQ(str.CStr(), "");
}

TEST(FStringTest, ConstructFromCStr)
{
    FString str("Hello");
    EXPECT_EQ(str.ByteLength(), 5);
    EXPECT_FALSE(str.Empty());
    EXPECT_STREQ(str.CStr(), "Hello");
}

TEST(FStringTest, ConstructFromNullCStr)
{
    FString str(nullptr);
    EXPECT_TRUE(str.Empty());
    EXPECT_EQ(str.ByteLength(), 0);
}

TEST(FStringTest, ConstructFromStringView)
{
    std::string_view sv = "Hello";
    FString str(sv);
    EXPECT_EQ(str.ByteLength(), 5);
    EXPECT_STREQ(str.CStr(), "Hello");
}

TEST(FStringTest, ConstructFromStdString)
{
    std::string s = "Hello";
    FString str(s);
    EXPECT_EQ(str.ByteLength(), 5);
    EXPECT_STREQ(str.CStr(), "Hello");
}

TEST(FStringTest, CopyConstructor)
{
    FString a("Hello");
    FString b(a);
    EXPECT_EQ(b.ByteLength(), a.ByteLength());
    EXPECT_STREQ(b.CStr(), "Hello");
}

TEST(FStringTest, MoveConstructor)
{
    FString a("Hello");
    FString b(std::move(a));
    EXPECT_STREQ(b.CStr(), "Hello");
    EXPECT_TRUE(a.Empty());
}

TEST(FStringTest, CopyAssignment)
{
    FString a("Hello");
    FString b;
    b = a;
    EXPECT_STREQ(b.CStr(), "Hello");
    EXPECT_STREQ(a.CStr(), "Hello"); // a unchanged
}

TEST(FStringTest, MoveAssignment)
{
    FString a("Hello");
    FString b;
    b = std::move(a);
    EXPECT_STREQ(b.CStr(), "Hello");
    EXPECT_TRUE(a.Empty());
}

TEST(FStringTest, AssignFromCStr)
{
    FString str;
    str = "World";
    EXPECT_STREQ(str.CStr(), "World");
    EXPECT_EQ(str.ByteLength(), 5);
}

TEST(FStringTest, AssignFromStringView)
{
    FString str;
    str = std::string_view("World");
    EXPECT_STREQ(str.CStr(), "World");
}

// SSO boundary: 15 bytes inline, 16+ goes to heap
TEST(FStringTest, SSOBoundaryInline)
{
    FString str("123456789012345"); // exactly 15 bytes
    EXPECT_EQ(str.ByteLength(), 15);
    EXPECT_STREQ(str.CStr(), "123456789012345");
}

TEST(FStringTest, SSOBoundaryHeap)
{
    FString str("1234567890123456"); // 16 bytes, goes to heap
    EXPECT_EQ(str.ByteLength(), 16);
    EXPECT_STREQ(str.CStr(), "1234567890123456");
}

TEST(FStringTest, AppendOperatorPlusEquals)
{
    FString str("Hello");
    str += " World";
    EXPECT_STREQ(str.CStr(), "Hello World");
    EXPECT_EQ(str.ByteLength(), 11);
}

TEST(FStringTest, AppendOperatorPlus)
{
    FString a("Hello");
    FString b = a + " World";
    EXPECT_STREQ(b.CStr(), "Hello World");
    EXPECT_STREQ(a.CStr(), "Hello"); // a unchanged
}

TEST(FStringTest, AppendCausesSSoToHeapTransition)
{
    FString str("Hello!!");    // 7 bytes, SSO
    str += " This is a long string"; // pushes past 15 bytes, triggers heap
    EXPECT_STREQ(str.CStr(), "Hello!! This is a long string");
}

TEST(FStringTest, EqualityWithFString)
{
    FString a("Hello");
    FString b("Hello");
    FString c("World");
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST(FStringTest, EqualityWithCStr)
{
    FString str("Hello");
    EXPECT_EQ(str, "Hello");
    EXPECT_NE(str, "World");
}

TEST(FStringTest, EqualityWithStringView)
{
    FString str("Hello");
    EXPECT_EQ(str, std::string_view("Hello"));
    EXPECT_NE(str, std::string_view("World"));
}

TEST(FStringTest, View)
{
    FString str("Hello");
    std::string_view view = str.View();
    EXPECT_EQ(view, "Hello");
    EXPECT_EQ(view.size(), 5);
}

TEST(FStringTest, ToStdString)
{
    FString str("Hello");
    std::string s = str.ToStdString();
    EXPECT_EQ(s, "Hello");
}

// UTF-8: "café" — 4 codepoints, 5 bytes (é is 2 bytes in UTF-8)
TEST(FStringTest, UTF8ByteLengthVsCodepoints)
{
    FString str("café");
    std::ranges::distance(str.Codepoints());

    EXPECT_EQ(std::ranges::distance(str.Codepoints()), 4); // 4 codepoints
    EXPECT_EQ(str.ByteLength(), 5);                        // 5 bytes
}

// UTF-8: "こんにちは" — 5 codepoints, 15 bytes (3 bytes each in UTF-8)
TEST(FStringTest, UTF8JapaneseSSoBoundary)
{
    FString str("こんにちは");
    EXPECT_EQ(std::ranges::distance(str.Codepoints()), 5); // 5 codepoints
    EXPECT_EQ(str.ByteLength(), 15);                       // 15 bytes, exactly SSO limit
}

// UTF-8: "こんにちは!" — 6 codepoints, 16 bytes (heap)
TEST(FStringTest, UTF8JapaneseHeap)
{
    FString str("こんにちは!");
    EXPECT_EQ(std::ranges::distance(str.Codepoints()), 6);
    EXPECT_EQ(str.ByteLength(), 16);
}

TEST(FStringTest, UTF8CodepointsDecodeCorrectly)
{
    FString str("café");
    str += " latte";
    EXPECT_EQ(std::ranges::distance(str.Codepoints()), 10); // 4 + 6 codepoints
    EXPECT_EQ(str.ByteLength(), 11);                        // 5 + 6 bytes
}

#pragma endregion FString

#pragma region Ptr

class FTestObject : public FObject
{
    FUSION_CLASS(FTestObject, FObject)
public:
    std::atomic<int> m_Value = 0;
};

TEST(PtrTest, ThreadSafety)
{
    // Shared strong ref — all threads copy from this
    Ref<FTestObject> shared = new FTestObject();

    constexpr int threadCount    = 8;
    constexpr int opsPerThread   = 10000;

    std::vector<std::thread> threads;
    threads.reserve(threadCount * 2);

    // Strong ref threads: repeatedly copy and release Ptr<T>
    for (int i = 0; i < threadCount; ++i)
    {
        threads.emplace_back([&]()
        {
            for (int j = 0; j < opsPerThread; ++j)
            {
                Ref<FTestObject> local = shared;
                if (local)
                    local->m_Value.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    // Weak ref threads: repeatedly lock and release WeakPtr<T>
    WeakRef<FTestObject> weak = shared;
    for (int i = 0; i < threadCount; ++i)
    {
        threads.emplace_back([&]()
        {
            for (int j = 0; j < opsPerThread; ++j)
            {
                Ref<FTestObject> locked = weak.Lock();
                if (locked)
                    locked->m_Value.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& t : threads)
        t.join();

    // Object should still be alive — shared still holds a strong ref
    EXPECT_TRUE(shared.IsValid());
    EXPECT_EQ(shared->m_Value.load(), threadCount * opsPerThread * 2);

    // Release shared — object should be destroyed, weak should be dead
    shared = nullptr;
    EXPECT_FALSE(weak.IsValid());
    EXPECT_EQ(weak.Lock(), nullptr);
}

#pragma endregion Ptr

#pragma region FArray

TEST(FArrayTest, DefaultConstructorEmpty)
{
    FArray<int> arr;
    EXPECT_EQ(arr.Size(), 0);
    EXPECT_TRUE(arr.Empty());
    EXPECT_EQ(arr.Capacity(), 8); // default InlineCapacity
}

TEST(FArrayTest, InitializerList)
{
    FArray<int> arr = { 1, 2, 3 };
    EXPECT_EQ(arr.Size(), 3);
    EXPECT_EQ(arr[0], 1);
    EXPECT_EQ(arr[1], 2);
    EXPECT_EQ(arr[2], 3);
}

TEST(FArrayTest, AddAndAccess)
{
    FArray<int> arr;
    arr.Add(10);
    arr.Add(20);
    arr.Add(30);
    EXPECT_EQ(arr.Size(), 3);
    EXPECT_EQ(arr.First(), 10);
    EXPECT_EQ(arr.Last(), 30);
}

TEST(FArrayTest, EmplaceReturnsRef)
{
    FArray<std::string> arr;
    std::string& ref = arr.Emplace("hello");
    EXPECT_EQ(ref, "hello");
    EXPECT_EQ(arr.Size(), 1);
}

TEST(FArrayTest, Pop)
{
    FArray<int> arr = { 1, 2, 3 };
    arr.Pop();
    EXPECT_EQ(arr.Size(), 2);
    EXPECT_EQ(arr.Last(), 2);
}

TEST(FArrayTest, PopEmpty)
{
    FArray<int> arr;
    EXPECT_THROW(arr.Pop(), FException);
}

TEST(FArrayTest, RemoveAtOrdered)
{
    FArray<int> arr = { 10, 20, 30, 40 };
    arr.RemoveAt(1); // remove 20
    EXPECT_EQ(arr.Size(), 3);
    EXPECT_EQ(arr[0], 10);
    EXPECT_EQ(arr[1], 30);
    EXPECT_EQ(arr[2], 40);
}

TEST(FArrayTest, RemoveAtSwapUnordered)
{
    FArray<int> arr = { 10, 20, 30, 40 };
    arr.RemoveAtSwapLast(1); // swaps 20 with 40
    EXPECT_EQ(arr.Size(), 3);
    EXPECT_EQ(arr[0], 10);
    EXPECT_EQ(arr[1], 40);
    EXPECT_EQ(arr[2], 30);
}

TEST(FArrayTest, RemoveAtSwapLastElement)
{
    FArray<int> arr = { 10, 20, 30 };
    arr.RemoveAtSwapLast(2); // removing last — no swap needed
    EXPECT_EQ(arr.Size(), 2);
    EXPECT_EQ(arr[0], 10);
    EXPECT_EQ(arr[1], 20);
}

TEST(FArrayTest, Clear)
{
    FArray<int> arr = { 1, 2, 3 };
    arr.Clear();
    EXPECT_EQ(arr.Size(), 0);
    EXPECT_TRUE(arr.Empty());
}

TEST(FArrayTest, ReserveDoesNotShrink)
{
    FArray<int> arr;
    arr.Reserve(20);
    EXPECT_GE(arr.Capacity(), 20);
    arr.Reserve(5);
    EXPECT_GE(arr.Capacity(), 20); // should not shrink
}

TEST(FArrayTest, ResizeDefaultValue)
{
    FArray<int> arr;
    arr.Resize(5);
    EXPECT_EQ(arr.Size(), 5);
    for (size_t i = 0; i < arr.Size(); ++i)
        EXPECT_EQ(arr[i], 0);
}

TEST(FArrayTest, ResizeWithValue)
{
    FArray<int> arr;
    arr.Resize(4, 42);
    EXPECT_EQ(arr.Size(), 4);
    for (size_t i = 0; i < arr.Size(); ++i)
        EXPECT_EQ(arr[i], 42);
}

TEST(FArrayTest, ResizeShrink)
{
    FArray<int> arr = { 1, 2, 3, 4, 5 };
    arr.Resize(3);
    EXPECT_EQ(arr.Size(), 3);
    EXPECT_EQ(arr[2], 3);
}

TEST(FArrayTest, Contains)
{
    FArray<int> arr = { 10, 20, 30 };
    EXPECT_TRUE(arr.Contains(20));
    EXPECT_FALSE(arr.Contains(99));
}

TEST(FArrayTest, IndexOf)
{
    FArray<int> arr = { 10, 20, 30 };
    EXPECT_EQ(arr.IndexOf(20), 1);
    EXPECT_EQ(arr.IndexOf(99), FArray<int>::npos);
}

// SBO boundary: inline capacity is 8, adding a 9th element triggers heap growth
TEST(FArrayTest, SBOInlineBoundary)
{
    FArray<int> arr;
    for (int i = 0; i < 8; ++i)
        arr.Add(i);
    EXPECT_EQ(arr.Size(), 8);
    EXPECT_EQ(arr.Capacity(), 8);

    arr.Add(8); // triggers grow
    EXPECT_EQ(arr.Size(), 9);
    EXPECT_GE(arr.Capacity(), 9);

    for (int i = 0; i < 9; ++i)
        EXPECT_EQ(arr[i], i);
}

TEST(FArrayTest, CopyConstructor)
{
    FArray<int> a = { 1, 2, 3 };
    FArray<int> b(a);
    EXPECT_EQ(b.Size(), 3);
    EXPECT_EQ(b[0], 1);
    b[0] = 99;
    EXPECT_EQ(a[0], 1); // deep copy — a unchanged
}

TEST(FArrayTest, MoveConstructorInline)
{
    FArray<int> a = { 1, 2, 3 };
    FArray<int> b(std::move(a));
    EXPECT_EQ(b.Size(), 3);
    EXPECT_EQ(b[0], 1);
    EXPECT_EQ(a.Size(), 0); // a emptied
}

TEST(FArrayTest, MoveConstructorHeap)
{
    FArray<int> a;
    for (int i = 0; i < 16; ++i) // force heap
        a.Add(i);

    FArray<int> b(std::move(a));
    EXPECT_EQ(b.Size(), 16);
    EXPECT_EQ(b[0], 0);
    EXPECT_EQ(b[15], 15);
    EXPECT_EQ(a.Size(), 0);
}

TEST(FArrayTest, CopyAssignment)
{
    FArray<int> a = { 1, 2, 3 };
    FArray<int> b;
    b = a;
    EXPECT_EQ(b.Size(), 3);
    EXPECT_EQ(a.Size(), 3); // a unchanged
}

TEST(FArrayTest, MoveAssignment)
{
    FArray<int> a = { 1, 2, 3 };
    FArray<int> b;
    b = std::move(a);
    EXPECT_EQ(b.Size(), 3);
    EXPECT_EQ(a.Size(), 0);
}

TEST(FArrayTest, RangeFor)
{
    FArray<int> arr = { 1, 2, 3, 4 };
    int sum = 0;
    for (int v : arr)
        sum += v;
    EXPECT_EQ(sum, 10);
}

TEST(FArrayTest, CustomInlineCapacity)
{
    FArray<int, 2> arr;
    EXPECT_EQ(arr.Capacity(), 2);
    arr.Add(1);
    arr.Add(2);
    EXPECT_EQ(arr.Capacity(), 2);
    arr.Add(3); // goes to heap
    EXPECT_GE(arr.Capacity(), 3);
    EXPECT_EQ(arr.Size(), 3);
}

TEST(FArrayTest, ZeroInlineCapacity)
{
    FArray<int, 0> arr;
    EXPECT_EQ(arr.Capacity(), 0);
    arr.Add(1);
    EXPECT_EQ(arr.Size(), 1);
    EXPECT_EQ(arr[0], 1);
}

TEST(FArrayTest, NonTrivialType)
{
    FArray<FString> arr;
    arr.Add(FString("hello"));
    arr.Add(FString("world"));
    EXPECT_EQ(arr.Size(), 2);
    EXPECT_EQ(arr[0], "hello");
    EXPECT_EQ(arr[1], "world");

    arr.RemoveAt(0);
    EXPECT_EQ(arr.Size(), 1);
    EXPECT_EQ(arr[0], "world");
}

#pragma endregion FArray

#pragma region FVec2

TEST(FVec2Test, DefaultConstructor)
{
    FVec2 v;
    EXPECT_EQ(v.x, 0.0f);
    EXPECT_EQ(v.y, 0.0f);
}

TEST(FVec2Test, ScalarConstructor)
{
    FVec2 v(3.0f);
    EXPECT_EQ(v.x, 3.0f);
    EXPECT_EQ(v.y, 3.0f);
}

TEST(FVec2Test, XYConstructor)
{
    FVec2 v(1.0f, 2.0f);
    EXPECT_EQ(v.x, 1.0f);
    EXPECT_EQ(v.y, 2.0f);
}

TEST(FVec2Test, UnionAliases)
{
    FVec2 v(3.0f, 4.0f);
    EXPECT_EQ(v.width,  3.0f);
    EXPECT_EQ(v.height, 4.0f);
    EXPECT_EQ(v.left,   3.0f);
    EXPECT_EQ(v.right,  4.0f);
    EXPECT_EQ(v.top,    3.0f);
    EXPECT_EQ(v.bottom, 4.0f);
    EXPECT_EQ(v.xy[0],  3.0f);
    EXPECT_EQ(v.xy[1],  4.0f);
}

TEST(FVec2Test, StaticConstants)
{
    EXPECT_EQ(FVec2::Zero(), FVec2(0.0f, 0.0f));
    EXPECT_EQ(FVec2::One(),  FVec2(1.0f, 1.0f));
}

TEST(FVec2Test, Arithmetic)
{
    FVec2 a(1.0f, 2.0f);
    FVec2 b(3.0f, 4.0f);

    EXPECT_EQ(a + b, FVec2(4.0f, 6.0f));
    EXPECT_EQ(b - a, FVec2(2.0f, 2.0f));
    EXPECT_EQ(a * b, FVec2(3.0f, 8.0f));
    EXPECT_EQ(a * 2.0f, FVec2(2.0f, 4.0f));
    EXPECT_EQ(2.0f * a, FVec2(2.0f, 4.0f));
    EXPECT_EQ(b / 2.0f, FVec2(1.5f, 2.0f));
    EXPECT_EQ(-a, FVec2(-1.0f, -2.0f));
}

TEST(FVec2Test, CompoundAssignment)
{
    FVec2 v(2.0f, 4.0f);
    v += FVec2(1.0f, 1.0f);
    EXPECT_EQ(v, FVec2(3.0f, 5.0f));
    v -= FVec2(1.0f, 1.0f);
    EXPECT_EQ(v, FVec2(2.0f, 4.0f));
    v *= 2.0f;
    EXPECT_EQ(v, FVec2(4.0f, 8.0f));
    v /= 2.0f;
    EXPECT_EQ(v, FVec2(2.0f, 4.0f));
}

TEST(FVec2Test, Equality)
{
    EXPECT_EQ(FVec2(1.0f, 2.0f), FVec2(1.0f, 2.0f));
    EXPECT_NE(FVec2(1.0f, 2.0f), FVec2(1.0f, 3.0f));
}

TEST(FVec2Test, IndexOperator)
{
    FVec2 v(5.0f, 6.0f);
    EXPECT_EQ(v[0], 5.0f);
    EXPECT_EQ(v[1], 6.0f);
    v[0] = 9.0f;
    EXPECT_EQ(v.x, 9.0f);
}

TEST(FVec2Test, IndexOutOfBounds)
{
    FVec2 v(1.0f, 2.0f);
    EXPECT_THROW(v[2], FException);
}

TEST(FVec2Test, SqrMagnitude)
{
    FVec2 v(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(v.GetSqrMagnitude(), 25.0f);
}

TEST(FVec2Test, Magnitude)
{
    FVec2 v(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(v.GetMagnitude(), 5.0f);
}

TEST(FVec2Test, Normalized)
{
    FVec2 v(3.0f, 0.0f);
    EXPECT_FLOAT_EQ(v.GetNormalized().GetMagnitude(), 1.0f);
}

TEST(FVec2Test, NormalizedZeroVector)
{
    // Zero vector should not crash — returns zero
    EXPECT_EQ(FVec2::Zero().GetNormalized(), FVec2(0.0f, 0.0f));
}

TEST(FVec2Test, Dot)
{
    EXPECT_FLOAT_EQ(FVec2::Dot({ 1.0f, 0.0f }, { 0.0f, 1.0f }), 0.0f); // perpendicular
    EXPECT_FLOAT_EQ(FVec2::Dot({ 1.0f, 0.0f }, { 1.0f, 0.0f }), 1.0f); // parallel
}

TEST(FVec2Test, Distance)
{
    EXPECT_FLOAT_EQ(FVec2::Distance({ 0.0f, 0.0f }, { 3.0f, 4.0f }), 5.0f);
}

TEST(FVec2Test, MinMax)
{
    FVec2 a(1.0f, 5.0f);
    FVec2 b(3.0f, 2.0f);
    EXPECT_EQ(FVec2::Min(a, b), FVec2(1.0f, 2.0f));
    EXPECT_EQ(FVec2::Max(a, b), FVec2(3.0f, 5.0f));
}

TEST(FVec2Test, Lerp)
{
    FVec2 from(0.0f, 0.0f);
    FVec2 to(10.0f, 20.0f);
    EXPECT_EQ(FVec2::Lerp(from, to, 0.5f), FVec2(5.0f, 10.0f));
    EXPECT_EQ(FVec2::Lerp(from, to, 0.0f), from);
    EXPECT_EQ(FVec2::Lerp(from, to, 1.0f), to);
}

#pragma endregion FVec2

#pragma region FVec2i

TEST(FVec2iTest, DefaultConstructor)
{
    FVec2i v;
    EXPECT_EQ(v.x, 0);
    EXPECT_EQ(v.y, 0);
}

TEST(FVec2iTest, XYConstructor)
{
    FVec2i v(3, 7);
    EXPECT_EQ(v.x, 3);
    EXPECT_EQ(v.y, 7);
}

TEST(FVec2iTest, UnionAliases)
{
    FVec2i v(8, 16);
    EXPECT_EQ(v.width,  8);
    EXPECT_EQ(v.height, 16);
    EXPECT_EQ(v.xy[0],  8);
    EXPECT_EQ(v.xy[1],  16);
}

TEST(FVec2iTest, Arithmetic)
{
    FVec2i a(2, 3);
    FVec2i b(4, 5);
    EXPECT_EQ(a + b, FVec2i(6, 8));
    EXPECT_EQ(b - a, FVec2i(2, 2));
    EXPECT_EQ(a * b, FVec2i(8, 15));
    EXPECT_EQ(a * 3, FVec2i(6, 9));
    EXPECT_EQ(3 * a, FVec2i(6, 9));
    EXPECT_EQ(b / 2, FVec2i(2, 2));
    EXPECT_EQ(-a,    FVec2i(-2, -3));
}

TEST(FVec2iTest, IndexOutOfBounds)
{
    FVec2i v(1, 2);
    EXPECT_THROW(v[2], FException);
}

TEST(FVec2iTest, ConversionToFVec2)
{
    FVec2i vi(3, 4);
    FVec2  vf = static_cast<FVec2>(vi);
    EXPECT_FLOAT_EQ(vf.x, 3.0f);
    EXPECT_FLOAT_EQ(vf.y, 4.0f);
}

TEST(FVec2iTest, ConversionFromFVec2Truncates)
{
    FVec2  vf(3.9f, 4.1f);
    FVec2i vi(vf);
    EXPECT_EQ(vi.x, 3);
    EXPECT_EQ(vi.y, 4);
}

TEST(FVec2iTest, MinMax)
{
    FVec2i a(1, 9);
    FVec2i b(5, 3);
    EXPECT_EQ(FVec2i::Min(a, b), FVec2i(1, 3));
    EXPECT_EQ(FVec2i::Max(a, b), FVec2i(5, 9));
}

#pragma endregion FVec2i

#pragma region FRect

TEST(FRectTest, DefaultConstructor)
{
    FRect r;
    EXPECT_EQ(r.left,   0.0f);
    EXPECT_EQ(r.top,    0.0f);
    EXPECT_EQ(r.right,  0.0f);
    EXPECT_EQ(r.bottom, 0.0f);
    EXPECT_TRUE(r.IsEmpty());
}

TEST(FRectTest, ConstructFromLTRB)
{
    FRect r(1.0f, 2.0f, 5.0f, 6.0f);
    EXPECT_EQ(r.left,   1.0f);
    EXPECT_EQ(r.top,    2.0f);
    EXPECT_EQ(r.right,  5.0f);
    EXPECT_EQ(r.bottom, 6.0f);
}

TEST(FRectTest, ConstructFromMinMax)
{
    FRect r(FVec2(1.0f, 2.0f), FVec2(5.0f, 6.0f));
    EXPECT_EQ(r.min, FVec2(1.0f, 2.0f));
    EXPECT_EQ(r.max, FVec2(5.0f, 6.0f));
}

TEST(FRectTest, UnionLayoutMatchesLTRB)
{
    // Verify memory aliases are correct
    FRect r(1.0f, 2.0f, 5.0f, 6.0f);
    EXPECT_EQ(r.min.x, r.left);
    EXPECT_EQ(r.min.y, r.top);
    EXPECT_EQ(r.max.x, r.right);
    EXPECT_EQ(r.max.y, r.bottom);
}

TEST(FRectTest, FromSize)
{
    FRect r = FRect::FromSize(FVec2(1.0f, 2.0f), FVec2(4.0f, 6.0f));
    EXPECT_EQ(r.left,   1.0f);
    EXPECT_EQ(r.top,    2.0f);
    EXPECT_EQ(r.right,  5.0f);
    EXPECT_EQ(r.bottom, 8.0f);
}

TEST(FRectTest, FromSizeFloats)
{
    FRect r = FRect::FromSize(1.0f, 2.0f, 4.0f, 6.0f);
    EXPECT_EQ(r.right,  5.0f);
    EXPECT_EQ(r.bottom, 8.0f);
}

TEST(FRectTest, GetSizeAndCenter)
{
    FRect r(2.0f, 4.0f, 8.0f, 10.0f);
    EXPECT_EQ(r.GetSize(),   FVec2(6.0f, 6.0f));
    EXPECT_EQ(r.GetWidth(),  6.0f);
    EXPECT_EQ(r.GetHeight(), 6.0f);
    EXPECT_EQ(r.GetCenter(), FVec2(5.0f, 7.0f));
    EXPECT_FLOAT_EQ(r.GetArea(), 36.0f);
}

TEST(FRectTest, IsEmpty)
{
    EXPECT_TRUE(FRect(0.0f, 0.0f, 0.0f, 0.0f).IsEmpty());  // degenerate
    EXPECT_TRUE(FRect(5.0f, 0.0f, 3.0f, 1.0f).IsEmpty());  // inverted X
    EXPECT_FALSE(FRect(0.0f, 0.0f, 1.0f, 1.0f).IsEmpty());
}

TEST(FRectTest, ContainsPoint)
{
    FRect r(0.0f, 0.0f, 10.0f, 10.0f);
    EXPECT_TRUE(r.Contains({ 5.0f, 5.0f }));
    EXPECT_TRUE(r.Contains({ 0.0f, 0.0f }));   // border — inside
    EXPECT_TRUE(r.Contains({ 10.0f, 10.0f })); // border — inside
    EXPECT_FALSE(r.Contains({ 11.0f, 5.0f }));
}

TEST(FRectTest, Overlaps)
{
    FRect a(0.0f, 0.0f, 5.0f, 5.0f);
    FRect b(3.0f, 3.0f, 8.0f, 8.0f);
    FRect c(6.0f, 6.0f, 9.0f, 9.0f);
    EXPECT_TRUE(a.Overlaps(b));
    EXPECT_FALSE(a.Overlaps(c));
}

TEST(FRectTest, Translate)
{
    FRect r(1.0f, 2.0f, 4.0f, 6.0f);
    FRect t = r.Translate({ 1.0f, 2.0f });
    EXPECT_EQ(t.left,   2.0f);
    EXPECT_EQ(t.top,    4.0f);
    EXPECT_EQ(t.right,  5.0f);
    EXPECT_EQ(t.bottom, 8.0f);
}

TEST(FRectTest, Expand)
{
    FRect r(2.0f, 3.0f, 8.0f, 9.0f);
    FRect e = r.Expand(1.0f);
    EXPECT_EQ(e.left,   1.0f);
    EXPECT_EQ(e.top,    2.0f);
    EXPECT_EQ(e.right,  9.0f);
    EXPECT_EQ(e.bottom, 10.0f);
}

TEST(FRectTest, EncapsulatePoint)
{
    FRect r(1.0f, 1.0f, 5.0f, 5.0f);
    FRect e = r.Encapsulate(FVec2{ 8.0f, 0.0f });
    EXPECT_EQ(e.left,   1.0f);
    EXPECT_EQ(e.top,    0.0f);
    EXPECT_EQ(e.right,  8.0f);
    EXPECT_EQ(e.bottom, 5.0f);
}

TEST(FRectTest, EncapsulateRect)
{
    FRect a(0.0f, 0.0f, 4.0f, 4.0f);
    FRect b(2.0f, 2.0f, 8.0f, 8.0f);
    FRect e = a.Encapsulate(b);
    EXPECT_EQ(e, FRect(0.0f, 0.0f, 8.0f, 8.0f));
}

TEST(FRectTest, Union)
{
    FRect a(0.0f, 0.0f, 4.0f, 4.0f);
    FRect b(2.0f, 2.0f, 8.0f, 8.0f);
    FRect u = FRect::Union(a, b);
    EXPECT_EQ(u, FRect(0.0f, 0.0f, 8.0f, 8.0f));
}

TEST(FRectTest, UnionWithEmpty)
{
    FRect a(1.0f, 1.0f, 5.0f, 5.0f);
    FRect empty;
    EXPECT_EQ(FRect::Union(a, empty), a);
    EXPECT_EQ(FRect::Union(empty, a), a);
}

TEST(FRectTest, Intersection)
{
    FRect a(0.0f, 0.0f, 6.0f, 6.0f);
    FRect b(3.0f, 3.0f, 9.0f, 9.0f);
    FRect i = FRect::Intersection(a, b);
    EXPECT_EQ(i, FRect(3.0f, 3.0f, 6.0f, 6.0f));
}

TEST(FRectTest, IntersectionNoOverlap)
{
    FRect a(0.0f, 0.0f, 3.0f, 3.0f);
    FRect b(5.0f, 5.0f, 9.0f, 9.0f);
    EXPECT_TRUE(FRect::Intersection(a, b).IsEmpty());
}

TEST(FRectTest, Equality)
{
    EXPECT_EQ(FRect(1.0f, 2.0f, 3.0f, 4.0f), FRect(1.0f, 2.0f, 3.0f, 4.0f));
    EXPECT_NE(FRect(1.0f, 2.0f, 3.0f, 4.0f), FRect(1.0f, 2.0f, 3.0f, 5.0f));
}

#pragma endregion FRect

#pragma region FColor

TEST(FColorTest, DefaultConstructor)
{
    FColor c;
    EXPECT_EQ(c.r, 0.0f);
    EXPECT_EQ(c.g, 0.0f);
    EXPECT_EQ(c.b, 0.0f);
    EXPECT_EQ(c.a, 0.0f);
}

TEST(FColorTest, RGBAConstructor)
{
    FColor c(0.1f, 0.2f, 0.3f, 0.4f);
    EXPECT_FLOAT_EQ(c.r, 0.1f);
    EXPECT_FLOAT_EQ(c.g, 0.2f);
    EXPECT_FLOAT_EQ(c.b, 0.3f);
    EXPECT_FLOAT_EQ(c.a, 0.4f);
}

TEST(FColorTest, DefaultAlphaIsOne)
{
    FColor c(1.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(c.a, 1.0f);
}

TEST(FColorTest, MemoryLayoutIsABGR)
{
    // Verify in-memory order is [a][b][g][r]
    FColor c(1.0f, 0.0f, 0.0f, 1.0f); // red, full alpha
    EXPECT_FLOAT_EQ(c.abgr[0], 1.0f); // a
    EXPECT_FLOAT_EQ(c.abgr[1], 0.0f); // b
    EXPECT_FLOAT_EQ(c.abgr[2], 0.0f); // g
    EXPECT_FLOAT_EQ(c.abgr[3], 1.0f); // r
}

TEST(FColorTest, IndexOperatorIsRGBAOrdered)
{
    FColor c(0.1f, 0.2f, 0.3f, 0.4f);
    EXPECT_FLOAT_EQ(c[0], 0.1f); // r
    EXPECT_FLOAT_EQ(c[1], 0.2f); // g
    EXPECT_FLOAT_EQ(c[2], 0.3f); // b
    EXPECT_FLOAT_EQ(c[3], 0.4f); // a
}

TEST(FColorTest, IndexOutOfBounds)
{
    FColor c;
    EXPECT_THROW(c[4], FException);
}

TEST(FColorTest, RGBA8)
{
    FColor c = FColor::RGBA8(255, 128, 0, 255);
    EXPECT_FLOAT_EQ(c.r, 1.0f);
    EXPECT_NEAR(c.g, 128.0f / 255.0f, 1e-5f);
    EXPECT_FLOAT_EQ(c.b, 0.0f);
    EXPECT_FLOAT_EQ(c.a, 1.0f);
}

TEST(FColorTest, RGBHex)
{
    FColor c = FColor::RGBHex(0xFF8800);
    EXPECT_FLOAT_EQ(c.r, 1.0f);
    EXPECT_NEAR(c.g, 0x88 / 255.0f, 1e-5f);
    EXPECT_FLOAT_EQ(c.b, 0.0f);
    EXPECT_FLOAT_EQ(c.a, 1.0f);
}

TEST(FColorTest, RGBAHex)
{
    FColor c = FColor::RGBAHex(0xFF8800FF);
    EXPECT_FLOAT_EQ(c.r, 1.0f);
    EXPECT_NEAR(c.g, 0x88 / 255.0f, 1e-5f);
    EXPECT_FLOAT_EQ(c.b, 0.0f);
    EXPECT_FLOAT_EQ(c.a, 1.0f);
}

TEST(FColorTest, ToU32RoundTrip)
{
    FColor  original = FColor::RGBA8(255, 128, 64, 200);
    uint32_t packed  = original.ToU32();

    EXPECT_EQ((packed & 0xFF),         255u); // R
    EXPECT_EQ(((packed >> 8)  & 0xFF), 128u); // G
    EXPECT_EQ(((packed >> 16) & 0xFF),  64u); // B
    EXPECT_EQ(((packed >> 24) & 0xFF), 200u); // A
}

TEST(FColorTest, WithAlpha)
{
    FColor c = FColors::Red.WithAlpha(0.5f);
    EXPECT_FLOAT_EQ(c.r, 1.0f);
    EXPECT_FLOAT_EQ(c.a, 0.5f);
}

TEST(FColorTest, MultiplyByScalar)
{
    FColor c(1.0f, 0.5f, 0.25f, 1.0f);
    FColor result = c * 0.5f;
    EXPECT_FLOAT_EQ(result.r, 0.5f);
    EXPECT_FLOAT_EQ(result.g, 0.25f);
    EXPECT_FLOAT_EQ(result.b, 0.125f);
    EXPECT_FLOAT_EQ(result.a, 0.5f);
}

TEST(FColorTest, Equality)
{
    EXPECT_EQ(FColors::Red, FColor(1.0f, 0.0f, 0.0f, 1.0f));
    EXPECT_NE(FColors::Red, FColors::Blue);
}

TEST(FColorTest, Lerp)
{
    FColor result = FColor::Lerp(FColors::Black, FColors::White, 0.5f);
    EXPECT_FLOAT_EQ(result.r, 0.5f);
    EXPECT_FLOAT_EQ(result.g, 0.5f);
    EXPECT_FLOAT_EQ(result.b, 0.5f);
    EXPECT_FLOAT_EQ(result.a, 1.0f);
}

TEST(FColorTest, HSV)
{
    // Pure red in HSV is (0, 1, 1)
    FColor c = FColor::HSV(0.0f, 1.0f, 1.0f);
    EXPECT_NEAR(c.r, 1.0f, 1e-5f);
    EXPECT_NEAR(c.g, 0.0f, 1e-5f);
    EXPECT_NEAR(c.b, 0.0f, 1e-5f);
}

TEST(FColorTest, PredefinedColors)
{
    EXPECT_EQ(FColors::White, FColor(1.0f, 1.0f, 1.0f, 1.0f));
    EXPECT_EQ(FColors::Black, FColor(0.0f, 0.0f, 0.0f, 1.0f));
    EXPECT_EQ(FColors::Clear, FColor(0.0f, 0.0f, 0.0f, 0.0f));
}

#pragma endregion FColor

#pragma region FStableDynamicArray

TEST(FStableDynamicArrayTest, DefaultConstructor)
{
    FStableDynamicArray<int> arr;
    EXPECT_TRUE(arr.IsEmpty());
    EXPECT_EQ(arr.GetCount(), 0);
    EXPECT_EQ(arr.GetCapacity(), 0);
    EXPECT_EQ(arr.GetData(), nullptr);
}

TEST(FStableDynamicArrayTest, InsertAndCount)
{
    FStableDynamicArray<int> arr;
    arr.Insert(1);
    arr.Insert(2);
    arr.Insert(3);
    EXPECT_EQ(arr.GetCount(), 3);
    EXPECT_FALSE(arr.IsEmpty());
    EXPECT_EQ(arr[0], 1);
    EXPECT_EQ(arr[1], 2);
    EXPECT_EQ(arr[2], 3);
}

TEST(FStableDynamicArrayTest, GrowthIncrement)
{
    FStableDynamicArray<int, 4> arr;
    arr.Insert(1);
    arr.Insert(2);
    arr.Insert(3);
    arr.Insert(4);
    EXPECT_EQ(arr.GetCapacity(), 4);
    // Inserting beyond capacity triggers Grow()
    arr.Insert(5);
    EXPECT_GE(arr.GetCapacity(), 5);
    EXPECT_EQ(arr.GetCount(), 5);
    EXPECT_EQ(arr[4], 5);
}

TEST(FStableDynamicArrayTest, Reserve)
{
    FStableDynamicArray<int> arr;
    arr.Reserve(64);
    EXPECT_GE(arr.GetCapacity(), 64);
    EXPECT_EQ(arr.GetCount(), 0);

    // Reserve smaller than current capacity is a no-op
    arr.Reserve(10);
    EXPECT_GE(arr.GetCapacity(), 64);
}

TEST(FStableDynamicArrayTest, ReservePreservesData)
{
    FStableDynamicArray<int, 4> arr;
    arr.Insert(10);
    arr.Insert(20);
    arr.Reserve(64);
    EXPECT_EQ(arr.GetCount(), 2);
    EXPECT_EQ(arr[0], 10);
    EXPECT_EQ(arr[1], 20);
}

TEST(FStableDynamicArrayTest, FirstAndLast)
{
    FStableDynamicArray<int> arr;
    arr.Insert(10);
    arr.Insert(20);
    arr.Insert(30);
    EXPECT_EQ(arr.First(), 10);
    EXPECT_EQ(arr.Last(), 30);
}

TEST(FStableDynamicArrayTest, RemoveAll)
{
    FStableDynamicArray<int> arr;
    arr.Insert(1);
    arr.Insert(2);
    arr.RemoveAll();
    EXPECT_EQ(arr.GetCount(), 0);
    EXPECT_TRUE(arr.IsEmpty());
    // Capacity is retained
    EXPECT_GT(arr.GetCapacity(), 0);
}

TEST(FStableDynamicArrayTest, RemoveAt)
{
    FStableDynamicArray<int> arr;
    arr.Insert(1);
    arr.Insert(2);
    arr.Insert(3);
    arr.RemoveAt(1);
    EXPECT_EQ(arr.GetCount(), 2);
    EXPECT_EQ(arr[0], 1);
    EXPECT_EQ(arr[1], 3);
}

TEST(FStableDynamicArrayTest, RemoveAtFirst)
{
    FStableDynamicArray<int> arr;
    arr.Insert(10);
    arr.Insert(20);
    arr.Insert(30);
    arr.RemoveAt(0);
    EXPECT_EQ(arr.GetCount(), 2);
    EXPECT_EQ(arr[0], 20);
    EXPECT_EQ(arr[1], 30);
}

TEST(FStableDynamicArrayTest, RemoveAtOutOfBounds)
{
    FStableDynamicArray<int> arr;
    arr.Insert(1);
    arr.RemoveAt(5); // should not crash, no-op
    EXPECT_EQ(arr.GetCount(), 1);
}

TEST(FStableDynamicArrayTest, RemoveLast)
{
    FStableDynamicArray<int> arr;
    arr.Insert(1);
    arr.Insert(2);
    arr.Insert(3);
    arr.RemoveLast();
    EXPECT_EQ(arr.GetCount(), 2);
    EXPECT_EQ(arr.Last(), 2);
}

TEST(FStableDynamicArrayTest, InsertRange)
{
    FStableDynamicArray<int> arr;
    arr.InsertRange(3, 42);
    EXPECT_EQ(arr.GetCount(), 3);
    EXPECT_EQ(arr[0], 42);
    EXPECT_EQ(arr[1], 42);
    EXPECT_EQ(arr[2], 42);
}

TEST(FStableDynamicArrayTest, InsertMultipleValues)
{
    FStableDynamicArray<int> arr;
    int values[] = { 5, 10, 15 };
    arr.Insert(values, 3);
    EXPECT_EQ(arr.GetCount(), 3);
    EXPECT_EQ(arr[0], 5);
    EXPECT_EQ(arr[1], 10);
    EXPECT_EQ(arr[2], 15);
}

TEST(FStableDynamicArrayTest, GetByteSize)
{
    FStableDynamicArray<int> arr;
    arr.Insert(1);
    arr.Insert(2);
    EXPECT_EQ(arr.GetByteSize(), 2 * sizeof(int));
}

TEST(FStableDynamicArrayTest, CopyConstructor)
{
    FStableDynamicArray<int> a;
    a.Insert(1);
    a.Insert(2);
    a.Insert(3);

    FStableDynamicArray<int> b(a);
    EXPECT_EQ(b.GetCount(), 3);
    EXPECT_EQ(b[0], 1);
    EXPECT_EQ(b[1], 2);
    EXPECT_EQ(b[2], 3);

    // Ensure deep copy — modifying b doesn't affect a
    b.RemoveAt(0);
    EXPECT_EQ(a.GetCount(), 3);
}

TEST(FStableDynamicArrayTest, CopyAssignment)
{
    FStableDynamicArray<int> a;
    a.Insert(10);
    a.Insert(20);

    FStableDynamicArray<int> b;
    b.Insert(99);
    b = a;

    EXPECT_EQ(b.GetCount(), 2);
    EXPECT_EQ(b[0], 10);
    EXPECT_EQ(b[1], 20);
    EXPECT_EQ(a.GetCount(), 2); // a unchanged
}

TEST(FStableDynamicArrayTest, MoveConstructor)
{
    FStableDynamicArray<int> a;
    a.Insert(1);
    a.Insert(2);

    FStableDynamicArray<int> b(std::move(a));
    EXPECT_EQ(b.GetCount(), 2);
    EXPECT_EQ(b[0], 1);
    EXPECT_TRUE(a.IsEmpty());
    EXPECT_EQ(a.GetData(), nullptr);
}

TEST(FStableDynamicArrayTest, MoveAssignment)
{
    FStableDynamicArray<int> a;
    a.Insert(1);
    a.Insert(2);

    FStableDynamicArray<int> b;
    b = std::move(a);
    EXPECT_EQ(b.GetCount(), 2);
    EXPECT_TRUE(a.IsEmpty());
    EXPECT_EQ(a.GetData(), nullptr);
}

TEST(FStableDynamicArrayTest, SelfCopyAssignment)
{
    FStableDynamicArray<int> a;
    a.Insert(1);
    a = a;
    EXPECT_EQ(a.GetCount(), 1);
    EXPECT_EQ(a[0], 1);
}

TEST(FStableDynamicArrayTest, SelfMoveAssignment)
{
    FStableDynamicArray<int> a;
    a.Insert(1);
    a = std::move(a);
    EXPECT_EQ(a.GetCount(), 1);
}

TEST(FStableDynamicArrayTest, RangeBasedFor)
{
    FStableDynamicArray<int> arr;
    arr.Insert(10);
    arr.Insert(20);
    arr.Insert(30);

    int sum = 0;
    for (int v : arr)
        sum += v;
    EXPECT_EQ(sum, 60);
}

TEST(FStableDynamicArrayTest, Free)
{
    FStableDynamicArray<int> arr;
    arr.Insert(1);
    arr.Insert(2);
    arr.Free();
    EXPECT_TRUE(arr.IsEmpty());
    EXPECT_EQ(arr.GetCount(), 0);
    EXPECT_EQ(arr.GetCapacity(), 0);
    EXPECT_EQ(arr.GetData(), nullptr);
}

TEST(FStableDynamicArrayTest, InsertAfterFree)
{
    FStableDynamicArray<int> arr;
    arr.Insert(1);
    arr.Free();
    arr.Insert(2);
    EXPECT_EQ(arr.GetCount(), 1);
    EXPECT_EQ(arr[0], 2);
}

#pragma endregion FStableDynamicArray
