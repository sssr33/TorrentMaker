#pragma once

// D - void(*)(T*);
template<class T, class D> class ScopedValue {
public:
	ScopedValue() {
	}

	ScopedValue(const T &v, D deleter)
		: val(v), deleter(deleter) {
	}

	ScopedValue(T &&v, D deleter)
		: val(std::move(v)), deleter(deleter) {
	}

	~ScopedValue() {
		this->deleter(&this->val);
	}

	T *GetPtr() {
		return &this->val;
	}

	T &GetRef() {
		return this->val;
	}

private:
	T val;
	D deleter;
};

template<class T, class D>
ScopedValue<T, D> MakeScopedValue(const T &v, D deleter) {
	return ScopedValue<T, D>(v, deleter);
}