#pragma once

#include <wrl.h>
#include <array>
#include <type_traits>
#include <algorithm>

template<class T, size_t size>
class comptr_array {
	static_assert(std::is_base_of<IUnknown, T>::value, "T must inherit from IUnknown");
	typedef std::array<T *, size> containerType;
public:
	typedef typename containerType::pointer pointer;
	typedef typename containerType::const_pointer const_pointer;
	typedef typename containerType::const_reverse_iterator const_reverse_iterator;
	typedef typename containerType::const_iterator const_iterator;
	typedef typename containerType::const_reverse_iterator const_reverse_iterator;

	comptr_array() {
		this->container.fill(nullptr);
	}

	comptr_array(size_t size)
		: container(size, nullptr) {
	}

	comptr_array(const comptr_array &other)
		: container(other.container)
	{
		this->AddDataRef();
	}

	comptr_array(comptr_array &&other)
		: container(std::move(other.container)) {
	}

	~comptr_array() {
		this->ReleaseDataRef();
	}

	comptr_array &operator=(const comptr_array &other) {
		if (this != &other) {
			this->container = other.container;

			this->AddDataRef();
		}

		return *this;
	}

	comptr_array &operator=(comptr_array &&other) {
		if (this != &other) {
			this->container = std::move(other.container);
		}

		return *this;
	}

	T *get(size_t idx) const {
		return this->container[idx];
	}

	void set(size_t idx, T *v) {
		auto &dst = this->container[idx];

		if (v) {
			static_cast<IUnknown *>(v)->AddRef();
		}

		if (dst) {
			static_cast<IUnknown *>(dst)->Release();
		}

		dst = v;
	}

	void set(size_t idx, const Microsoft::WRL::ComPtr<T> &v) {
		auto tmp = v.Get();
		auto &dst = this->container[idx];

		if (tmp) {
			static_cast<IUnknown *>(tmp)->AddRef();
		}

		if (dst) {
			static_cast<IUnknown *>(dst)->Release();
		}

		dst = tmp;
	}

	size_t size() const {
		return this->container.size();
	}

	pointer data() {
		return this->container.data();
	}

	const_pointer data() const {
		return this->container.data();
	}

	const_iterator begin() const {
		return this->container.begin();
	}

	const_iterator end() const {
		return this->container.end();
	}

private:
	containerType container;

	void AddDataRef() {
		for (auto &i : this->container) {
			if (i) {
				static_cast<IUnknown *>(i)->AddRef();
			}
		}
	}

	void ReleaseDataRef() {
		for (auto &i : this->container) {
			if (i) {
				static_cast<IUnknown *>(i)->Release();
				i = nullptr;
			}
		}
	}
};