#pragma once

#include <libhelpers\H.h>

struct FindHandleDeleter {
	void operator()(HANDLE *v);
};