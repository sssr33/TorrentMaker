#pragma once
#include "DxHelpers.h"
#include "..\ResourceFactory\DxResources.h"

#include <libhelpers\Thread\PPL\critical_section_guard_unique.h>

class Dx {
public:
	DxDevice dev;
	DxResources res;
};

typedef critical_section_guard_unique<Dx> DxGuarded;