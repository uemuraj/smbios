#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <locale>
#include <iostream>
#include <vector>
#include <system_error>

#define SYSTEM_ERROR(ec) std::system_error(ec, std::system_category(), __FILE__ "(" _CRT_STRINGIZE(__LINE__) ")");


//
// GetSystemFirmwareTable() のリファレンスにサンプルコードがあるとおり SMBIOS データの取得は簡単です。
// ただし、ブートの後に更新されるような情報ではないらしいです。
// 単純に製品名などを参照したいだけなら WMI 経由の方が楽そうです。スクリプトでも叩けます。
// 
// また、手持ちのマザーボードでは期待していた "Cooling Device (Type 27)" の情報が見つかりません。
// BIOS が更新されることで取得できるようになる可能性はありますが...
//
// 同じ API で 'ACPI' のデータも取得できますが、データが１つしかない 'RSMB' （いつも ID=0）とは異なるため、
// 先に EnumSystemFirmwareTables() で ID を取得しておく必要があります。
// 
// 'ACPI' にも "Fan Objects" の定義がありますが、こちらも期待薄です。
// たぶんデバイスマネージャーで見えてなければ、おそらく見えないと考えられます。
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
