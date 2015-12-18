#pragma once

#include <libhelpers\H.h>

// just uncategorized helpers
// TODO maybe need to move them somewhere
class MiscHelpers {
public:
	static void EnumFiles(
		const std::wstring &folder,
		const std::function<void(const std::wstring &file)> &onFile);
};