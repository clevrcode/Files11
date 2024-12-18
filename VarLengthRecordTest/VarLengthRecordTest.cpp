#include "pch.h"
#include <Windows.h>
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
			char pwd[1024];
			GetCurrentDirectory((DWORD)sizeof(pwd), pwd);
			VarLengthRecord vlr;
			std::string fpath(pwd);
			fpath += "\\BATCH.BAT";
			bool result = vlr.IsVariableLengthRecordFile(fpath.c_str());
			Assert::AreEqual(true, result);
			// get length of file:
			int len = vlr.CalculateFileLength(fpath.c_str());
			Assert::AreEqual(0xdc, len);

			fpath = pwd;
			fpath += "\\CDRCSR.ZAV";
			result = vlr.IsVariableLengthRecordFile(fpath.c_str());
			Assert::AreEqual(true, result);
			// get length of file:
			len = vlr.CalculateFileLength(fpath.c_str());
			Assert::AreEqual(0x538, len); //??? 0x53a

			fpath = pwd;
			fpath += "\\1_5k.bin";
			result = vlr.IsVariableLengthRecordFile(fpath.c_str());
			Assert::AreEqual(false, result);
		}
	};
}
