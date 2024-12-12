#include "pch.h"
#include "Files11Base.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Files11BaseTest
{
	TEST_CLASS(Files11BaseTest)
	{
	public:
		
		TEST_METHOD(AsciiToRadix50Test)
		{
			std::string text;
			char* name1 = "BATCH";
			char* name2 = "BAT";
			uint16_t dest[3];
			Files11Base::AsciiToRadix50(name1, 9, dest);
			Assert::AreEqual((int)dest[0], 0x0cbc);
			Assert::AreEqual((int)dest[1], 0x1400);
			Assert::AreEqual((int)dest[2], 0x0000);
			Files11Base::Radix50ToAscii(dest, 3, text, true);
			Assert::AreEqual(text.c_str(), name1);
		}
	};
}
