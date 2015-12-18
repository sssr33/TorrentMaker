#include "MiscHelpers.h"

#include <iostream>
#include <libhelpers\ScopedValue.h>

void MiscHelpers::EnumFiles(
	const std::wstring &folder,
	const std::function<void(const std::wstring &file)> &onFile) 
{
	WIN32_FIND_DATAW findData;
	auto tmpFolder = folder + L"\\*";
	auto find = MakeScopedValue(FindFirstFileW(tmpFolder.c_str(), &findData), [](HANDLE *v) { FindClose(*v); });

	do {
		bool isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		bool badName1 = findData.cFileName[0] == '.' && findData.cFileName[1] == '\0';
		bool badName2 = findData.cFileName[0] == '.' && findData.cFileName[1] == '.' && findData.cFileName[2] == '\0';

		if (!isDir || (!badName1 && !badName2))
		{
			auto itemPath = folder + L'\\' + findData.cFileName;

			if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				EnumFiles(itemPath, onFile);
			}
			else {
				try {
					onFile(itemPath);
				}
				catch (...) {
					std::cout << "Failed to process ";
					std::wcout << itemPath;
					std::cout << " file." << std::endl;
				}
			}
		}
	} while (FindNextFileW(find.GetRef(), &findData));
}