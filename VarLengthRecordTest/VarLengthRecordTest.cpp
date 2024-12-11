#include "pch.h"
#include "CppUnitTest.h"

#include "..\include\VarLengthRecord.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace VarLengthRecordTest
{
	TEST_CLASS(VarLengthRecordTest)
	{
	public:
		
		TEST_METHOD(TestMethod1)
		{
			VarLengthRecord vlr;
			std::string fpath("BATCH.BAT");
			bool result = vlr.IsVariableLengthRecordFile(fpath.c_str());
			Assert::AreEqual(true, result);
			// get length of file:
			int len = vlr.CalculateFileLength(fpath.c_str());
			Assert::AreEqual(0xdc, len);
		}
	};
}
