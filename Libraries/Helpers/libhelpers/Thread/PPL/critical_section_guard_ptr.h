#pragma once
#include "critical_section_guard_container.h"
#include "..\..\Metainfo\PointerGetter.h"
#include "..\..\Metainfo\RawType.h"
#include "..\..\Metainfo\Marker.h"
#include "..\..\Macros.h"

#include <ppl.h>
#include <algorithm>
#include <memory>

// MultithreadWrapper must have constructor wchich accepts pointer to T
template<class T, class MultithreadWrapper = void, template<class T2, class M2> class AccessorType>
class critical_section_guard_ptr {
public:
	typedef AccessorType<typename RawType<T>::Type, typename RawType<MultithreadWrapper>::Type> Accessor;
	typedef critical_section_guard_ptr<T, MultithreadWrapper, typename AccessorType> Copy;

	critical_section_guard_ptr(const EmptyInit &v)
		: container(nullptr) {
	}

	critical_section_guard_ptr()
		: container(nullptr) {
	}

	critical_section_guard_ptr(critical_section_guard_container<T> *container)
		: container(container) {
	}

	Accessor Get() {
		return Accessor(this->container.get());
	}

	MultithreadWrapper GetMultithread() {
		return MultithreadWrapper(PointerGetter::Get(this->container->obj));
	}
private:
	critical_section_guard_container<T> *container;
};

template<class T, template<class T2, class M2> class AccessorType>
class critical_section_guard_ptr<T, void, AccessorType> {
public:
	typedef AccessorType<typename RawType<T>::Type, void> Accessor;
	typedef critical_section_guard_ptr<T, void, AccessorType> Copy;

	critical_section_guard_ptr(const EmptyInit &v)
		: container(nullptr) {
	}

	critical_section_guard_ptr()
		: container(nullptr) {
	}

	critical_section_guard_ptr(critical_section_guard_container<T> *container)
		: container(container) {
	}

	Accessor Get() {
		return Accessor(this->container.get());
	}

private:
	critical_section_guard_container<T> *container;
};