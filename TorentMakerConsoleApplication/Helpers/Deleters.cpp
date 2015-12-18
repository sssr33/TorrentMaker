#include "Deleters.h"

void FindHandleDeleter::operator()(HANDLE *v) {
	FindClose(*v);
}