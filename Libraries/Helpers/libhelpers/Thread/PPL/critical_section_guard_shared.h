#pragma once
#include "critical_section_guard_container.h"
#include "..\..\Metainfo\PointerGetter.h"
#include "..\..\Metainfo\RawType.h"
#include "..\..\Metainfo\Marker.h"
#include "..\..\Macros.h"

#include <ppl.h>
#include <algorithm>
#include <memory>

template<class T, class MultithreadWrapper>
class critical_section_guard_shared;

template<class T, class MultithreadWrapper>
class critical_section_guard_shared_accessor {
public:
	NO_COPY(critical_section_guard_shared_accessor);

	critical_section_guard_shared_accessor(const std::shared_ptr<critical_section_guard_container<T>> &container)
		: lock(container->cs), container(container) {
	}

	critical_section_guard_shared_accessor(critical_section_guard_shared_accessor &&other)
		: lock(std::move(other.lock)), container(std::move(other.container)) 
	{
		other.container = nullptr;
	}

	critical_section_guard_shared_accessor &operator=(critical_section_guard_shared_accessor &&other) {
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

	critical_section_guard_shared<T, MultithreadWrapper> GetGuard() {
		return critical_section_guard_shared<T, MultithreadWrapper>(this->container);
	}

private:
	critical_section_lock lock;
	std::shared_ptr<critical_section_guard_container<T>> container;
};




// MultithreadWrapper must have constructor wchich accepts pointer to T
template<class T, class MultithreadWrapper = void>
class critical_section_guard_shared {
public:
	typedef critical_section_guard_shared_accessor<typename RawType<T>::Type, typename RawType<MultithreadWrapper>::Type> Accessor;
	typedef critical_section_guard_shared<T, MultithreadWrapper> Copy;

	critical_section_guard_shared(const EmptyInit &v) {}

	critical_section_guard_shared() {
		this->container = std::make_shared<critical_section_guard_container<T>>();
	}

	critical_section_guard_shared(const std::shared_ptr<critical_section_guard_container<T>> &container) {
		this->container = container;
	}

	Accessor Get() {
		return Accessor(this->container);
	}

	MultithreadWrapper GetMultithread() {
		return MultithreadWrapper(PointerGetter::Get(this->container->obj));
	}
private:
	std::shared_ptr<critical_section_guard_container<T>> container;
};

template<class T>
class critical_section_guard_shared<T, void> {
public:
	typedef critical_section_guard_shared_accessor<typename RawType<T>::Type, void> Accessor;
	typedef critical_section_guard_shared<T, void> Copy;

	critical_section_guard_shared(const EmptyInit &v) {}

	critical_section_guard_shared() {
		this->container = std::make_shared<critical_section_guard_container<T>>();
	}

	critical_section_guard_shared(const std::shared_ptr<critical_section_guard_container<T>> &container) {
		this->container = container;
	}

	Accessor Get() {
		return Accessor(this->container);
	}

private:
	std::shared_ptr<critical_section_guard_container<T>> container;
};