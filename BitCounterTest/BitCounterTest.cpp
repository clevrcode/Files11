#include "pch.h"
#include "CppUnitTest.h"
#include "..\include\BitCounter.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace BitCounterTest
{
	TEST_CLASS(BitCounterTest)
	{
	public:
		
		TEST_METHOD(TestMethod1)
		{
			BitCounter counter;
			uint8_t data[8] = { 0x0f, 0xf0, 0xff, 0xff, 0xff, 0xc0, 0x01, 0x80 };
			counter.Count(data, 64);
			Assert::AreEqual(36, counter.GetNbHi());
			Assert::AreEqual(64 - 36, counter.GetNbLo());
			Assert::AreEqual(28, counter.GetLargestContiguousHi());
			Assert::AreEqual(14, counter.GetLargestContiguousLo());

			// Reset counter
			counter.Reset();
			Assert::AreEqual(0, counter.GetNbHi());
			Assert::AreEqual(0, counter.GetNbLo());
			Assert::AreEqual(0, counter.GetLargestContiguousHi());
			Assert::AreEqual(0, counter.GetLargestContiguousLo());

			// second run
			data[0] = 0xff;
			counter.Count(data, 64);
			Assert::AreEqual(40, counter.GetNbHi());
			Assert::AreEqual(64 - 40, counter.GetNbLo());
			Assert::AreEqual(28, counter.GetLargestContiguousHi());
			Assert::AreEqual(14, counter.GetLargestContiguousLo());
		}

		TEST_METHOD(TestMethod2)
		{
			BitCounter counter;
			uint8_t data[8] = { 0x0f, 0xf0, 0xff, 0xff, 0xff, 0xc0, 0x01, 0x80 };
			counter.Count(data, 60);
			Assert::AreEqual(35, counter.GetNbHi());
			Assert::AreEqual(60 - 35, counter.GetNbLo());
			Assert::AreEqual(28, counter.GetLargestContiguousHi());
			Assert::AreEqual(11, counter.GetLargestContiguousLo());

			// Reset counter
			counter.Reset();
			Assert::AreEqual(0, counter.GetNbHi());
			Assert::AreEqual(0, counter.GetNbLo());
			Assert::AreEqual(0, counter.GetLargestContiguousHi());
			Assert::AreEqual(0, counter.GetLargestContiguousLo());
		}

		TEST_METHOD(TestMethod3)
		{
			BitCounter counter;
			uint8_t data1[8] = { 0x0f, 0xf0, 0xff, 0xff, 0xff, 0xc0, 0x01, 0x80 };
			counter.Count(data1, 64);
			Assert::AreEqual(36, counter.GetNbHi());
			Assert::AreEqual(64 - 36, counter.GetNbLo());
			Assert::AreEqual(28, counter.GetLargestContiguousHi());
			Assert::AreEqual(14, counter.GetLargestContiguousLo());

			uint8_t data2[8] = { 0xff, 0xff, 0x0f };
			counter.Count(data2, 20);
			Assert::AreEqual(36 + 16 + 4, counter.GetNbHi());
			Assert::AreEqual(84 - (36 + 16 + 4), counter.GetNbLo());
			Assert::AreEqual(28, counter.GetLargestContiguousHi());
			Assert::AreEqual(14, counter.GetLargestContiguousLo());

			// Reset counter
			counter.Reset();
			Assert::AreEqual(0, counter.GetNbHi());
			Assert::AreEqual(0, counter.GetNbLo());
			Assert::AreEqual(0, counter.GetLargestContiguousHi());
			Assert::AreEqual(0, counter.GetLargestContiguousLo());
		}

	};
}
