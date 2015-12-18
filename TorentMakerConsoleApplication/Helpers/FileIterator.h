#pragma once
#include "Deleters.h"

#include <queue>
#include <libhelpers\H.h>
#include <libhelpers\ScopedValue.h>

class FileIterator {
	typedef ScopedValue<HANDLE, FindHandleDeleter> ScopedFind;
public:
	NO_COPY(FileIterator);

	FileIterator(const std::wstring &folder);
	FileIterator(FileIterator &&other);

	FileIterator &operator=(FileIterator &&other);

	bool Next();

	std::wstring GetCurrent() const;

private:
	ScopedFind find;
	WIN32_FIND_DATAW findData;
	std::queue<std::wstring> leftFolders;
	
	bool BadName() const;
	void CleanFileData();
	bool MoveNext();
};