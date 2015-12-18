#include "FileIterator.h"

FileIterator::FileIterator(const std::wstring &folder)
	: find(INVALID_HANDLE_VALUE, FindHandleDeleter())
{
	this->leftFolders.push(folder);

	this->CleanFileData();
}

FileIterator::FileIterator(FileIterator &&other)
	: find(std::move(other.find)), findData(std::move(other.findData)), 
	leftFolders(std::move(other.leftFolders))
{
}

FileIterator &FileIterator::operator=(FileIterator &&other) {
	if (this != &other) {
		this->find = std::move(other.find);
		this->findData = std::move(other.findData);
		this->leftFolders = std::move(other.leftFolders);
	}

	return *this;
}

bool FileIterator::Next() {
	bool isDir = true;
	bool have = true;

	while (have && isDir) {
		if (this->find.GetRef() != INVALID_HANDLE_VALUE) {
			have = this->MoveNext();

			if (!have) {
				// force move to next folder
				this->find = ScopedFind(INVALID_HANDLE_VALUE, FindHandleDeleter());

				this->leftFolders.pop();

				if (this->leftFolders.empty()) {
					// no folders left to check
					// just exit...
					break;
				}

				this->CleanFileData();
			}
		}

		if (this->find.GetRef() == INVALID_HANDLE_VALUE && !this->leftFolders.empty()) {
			auto tmpPath = this->leftFolders.front() + L"\\*";
			this->find = ScopedFind(FindFirstFileW(tmpPath.c_str(), &this->findData), FindHandleDeleter());

			have = this->find.GetRef() != INVALID_HANDLE_VALUE;
		}

		if (have) {
			isDir = (this->findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

			if (isDir && !this->BadName()) {
				auto folderPath = this->leftFolders.front() + L'\\' + this->findData.cFileName;
				this->leftFolders.push(folderPath);
			}
		}
	}

	return have;
}

std::wstring FileIterator::GetCurrent() const {
	auto folderPath = this->leftFolders.front() + L'\\' + this->findData.cFileName;
	return folderPath;
}

bool FileIterator::BadName() const {
	bool badName1 = this->findData.cFileName[0] == '.' 
		&& this->findData.cFileName[1] == '\0';

	bool badName2 = this->findData.cFileName[0] == '.' 
		&& this->findData.cFileName[1] == '.' 
		&& this->findData.cFileName[2] == '\0';

	bool badName = badName1 || badName2;

	return badName;
}

void FileIterator::CleanFileData() {
	this->findData.dwFileAttributes = 0;
	ZeroMemory(this->findData.cFileName, sizeof(this->findData.cFileName));
}

bool FileIterator::MoveNext() {
	bool canMoveNext = FindNextFileW(this->find.GetRef(), &this->findData) == TRUE;
	return canMoveNext;
}