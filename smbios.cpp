#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <locale>
#include <iostream>
#include <vector>
#include <system_error>

#define SYSTEM_ERROR(ec) std::system_error(ec, std::system_category(), __FILE__ "(" _CRT_STRINGIZE(__LINE__) ")");


//
// GetSystemFirmwareTable() �̃��t�@�����X�ɃT���v���R�[�h������Ƃ��� SMBIOS �f�[�^�̎擾�͊ȒP�ł��B
// �������A�u�[�g�̌�ɍX�V�����悤�ȏ��ł͂Ȃ��炵���ł��B
// �P���ɐ��i���Ȃǂ��Q�Ƃ����������Ȃ� WMI �o�R�̕����y�����ł��B�X�N���v�g�ł��@���܂��B
// 
// �܂��A�莝���̃}�U�[�{�[�h�ł͊��҂��Ă��� "Cooling Device (Type 27)" �̏�񂪌�����܂���B
// BIOS ���X�V����邱�ƂŎ擾�ł���悤�ɂȂ�\���͂���܂���...
//
// ���� API �� 'ACPI' �̃f�[�^���擾�ł��܂����A�f�[�^���P�����Ȃ� 'RSMB' �i���� ID=0�j�Ƃ͈قȂ邽�߁A
// ��� EnumSystemFirmwareTables() �� ID ���擾���Ă����K�v������܂��B
// 
// 'ACPI' �ɂ� "Fan Objects" �̒�`������܂����A����������Ҕ��ł��B
// ���Ԃ�f�o�C�X�}�l�[�W���[�Ō����ĂȂ���΁A�����炭�����Ȃ��ƍl�����܂��B
//
#pragma warning(push)
#pragma warning(disable:4200)
struct RawSMBIOSData
{
	BYTE    Used20CallingMethod;
	BYTE    SMBIOSMajorVersion;
	BYTE    SMBIOSMinorVersion;
	BYTE    DmiRevision;
	DWORD    Length;
	BYTE    SMBIOSTableData[];
};
#pragma warning(pop)

int wmain()
{
	std::locale::global(std::locale(""));

	const DWORD signature = 'RSMB';

	try
	{
		DWORD bufferSize = ::GetSystemFirmwareTable(signature, 0, nullptr, 0);

		if (bufferSize == 0)
		{
			throw SYSTEM_ERROR(::GetLastError());
		}

		std::vector<std::uint8_t> buffer(bufferSize, 0);

		if (::GetSystemFirmwareTable(signature, 0, buffer.data(), bufferSize) != bufferSize)
		{
			throw SYSTEM_ERROR(ERROR_INVALID_DATA);
		}

		auto data = (RawSMBIOSData *) buffer.data();

		std::cout << "Used20CallingMethod=" << (bool) data->Used20CallingMethod << std::endl;
		std::cout << "SMBIOSMajorVersion=" << (unsigned) data->SMBIOSMajorVersion << std::endl;
		std::cout << "SMBIOSMinorVersion=" << (unsigned) data->SMBIOSMinorVersion << std::endl;
		std::cout << "DmiRevision=" << (unsigned) data->DmiRevision << std::endl;
		std::cout << "Length=" << (unsigned) data->Length << std::endl;

		auto p = data->SMBIOSTableData;

		while (p[0] < 127 && p[1] > 0)
		{
			const auto type = p[0];

			std::wcout << L"Type" << type << L" {";

			p += p[1];

			if (p[0])
			{
				while (p[0])
				{
					auto str = (const char *) p;

					std::cout << str << ", ";

					p += (strlen(str) + 1);
				}

				++p;
			}
			else
			{
				p += 2;
			}

			std::cout << "}" << std::endl;
		}

		return 0;
	}
	catch (const std::exception & e)
	{
		::OutputDebugStringA(e.what());
		::OutputDebugStringA("\r\n");
	}

	return 1;
}
