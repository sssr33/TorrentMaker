#pragma once
#include "critical_section_lock.h"

template<class T>
struct critical_section_guard_container {
	Concurrency::critical_section cs;
	T obj;
};