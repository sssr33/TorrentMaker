#pragma once
#include "critical_section_guard_container.h"
#include "critical_section_guard_ptr.h"
#include "..\..\Metainfo\PointerGetter.h"
#include "..\..\Metainfo\RawType.h"
#include "..\..\Metainfo\Marker.h"
#include "..\..\Macros.h"

#include <ppl.h>
#include <algorithm>
#include <memory>

template<class T, class MultithreadWrapper>
class critical_section_guard_accessor;

template<class T, class MultithreadWrapper>
struct critical_section_guard_accessor_ptr {
	typedef critical_section_guard_ptr<T, MultithreadWrapper, critical_section_guard_accessor> Type;
};

template<class T, class MultithreadWrapper>
class critical_section_guard_accessor {
public:
	NO_COPY(critical_section_guard_accessor);

	critical_section_guard_accessor(critical_section_guard_container<T> *container)
		: lock(container->cs), container(container) {
	}

	critical_section_guard_accessor(critical_section_guard_accessor &&other)
		: lock(std::move(other.lock)), container(std::move(other.container))
	{
		other.container = nullptr;
	}

	critical_section_guard_accessor &operator=(critical_section_guard_accessor &&other) {
		if (this != &other) {
			this->lock = std::move(other.lock);
			this->container = std::move(other.container);

			other.container = nullptr;
		}

		return *this;
	}

	T *operator->() {
		return &this->container->obj;
	}

	const T *operator->() const {
		return &this->container->obj;
	}

	operator T*() {
		return &this->container->obj;
	}

	operator const T*() const {
		return &this->container->obj;
	}

	typename critical_section_guard_accessor_ptr<T, MultithreadWrapper>::Type GetGuard() {
		return critical_section_guard_accessor_ptr<T, MultithreadWrapper>::Type(this->container);
	}

private:
	critical_section_lock lock;
	critical_section_guard_container<T> *container;
};




// MultithreadWrapper must have constructor wchich accepts pointer to T
template<class T, class MultithreadWrapper = void>
class critical_section_guard {
public:
	typedef critical_section_guard_accessor<typename RawType<T>::Type, typename RawType<MultithreadWrapper>::Type> Accessor;
	typedef typename critical_section_guard_accessor_ptr<T, MultithreadWrapper>::Type Copy;

	NO_COPY(critical_section_guard);

	critical_section_guard(const EmptyInit &v) {}

	critical_section_guard() {
	}

	critical_section_guard(critical_section_guard &&other)
		: container(std::move(other.container)) {
	}

	Accessor Get() {
		return Accessor(&this->container);
	}

	MultithreadWrapper GetMultithread() {
		return MultithreadWrapper(PointerGetter::Get(&this->container.obj));
	}
private:
	critical_section_guard_container<T> container;
};

template<class T>
class critical_section_guard<T, void> {
public:
	typedef critical_section_guard_accessor<typename RawType<T>::Type, void> Accessor;
	typedef typename critical_section_guard_accessor_ptr<T, void>::Type Copy;

	NO_COPY(critical_section_guard);

	critical_section_guard(const EmptyInit &v) {}

	critical_section_guard() {
	}

	critical_section_guard(critical_section_guard &&other)
		: container(std::move(other.container)) {
	}

	Accessor Get() {
		return Accessor(&this->container);
	}

private:
	critical_section_guard_container<T> container;
};