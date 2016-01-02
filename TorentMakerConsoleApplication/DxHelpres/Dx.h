#pragma once
#include "DxDevice.h"
#include "..\ResourceFactory\DxResources.h" // BAD!!!

#include <libhelpers\Thread\PPL\critical_section_guard_unique.h>

class Dx {
public:
	DxDevice dev;
	DxResources res;
};

typedef critical_section_guard_unique<Dx> DxGuarded;