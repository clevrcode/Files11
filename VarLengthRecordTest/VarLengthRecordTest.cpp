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
			std::ifstream ifs;
			std::string fpath("BATCH.BAT");
			ifs.open(fpath.c_str(), std::ifstream::binary);
			Assert::IsTrue(ifs.good());
			if (ifs.good())
			{
				// get length of file:
				ifs.seekg(0, ifs.end);
				int dataSize = static_cast<int>(ifs.tellg());
				ifs.seekg(0, ifs.beg);
				Assert::AreEqual(0xc9, dataSize);
				int len = vlr.CalculateFileLength(ifs);
				Assert::AreEqual(0xdc, len);
			}
		}
	};
}
