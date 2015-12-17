#pragma once

#include <wrl.h>
#include <vector>
#include <type_traits>
#include <algorithm>

template<class T>
class comptr_vector{
	static_assert(std::is_base_of<IUnknown, T>::value, "T must inherit from IUnknown");
	typedef std::vector<T *> vecType;
public:
	typedef typename std::vector<T *>::pointer pointer;
	typedef typename std::vector<T *>::const_pointer const_pointer;
	typedef typename std::vector<T *>::const_reverse_iterator const_reverse_iterator;
	typedef typename std::vector<T *>::const_iterator const_iterator;
	typedef typename std::vector<T *>::const_reverse_iterator const_reverse_iterator;
	typedef typename std::vector<T *>::iterator iterator;
	typedef typename std::vector<T *>::reverse_iterator reverse_iterator;

	comptr_vector(){
	}

	comptr_vector(size_t size)
		: vec(size, nullptr){
	}

	comptr_vector(const comptr_vector &other)
		: vec(other.vec)
	{
		this->AddDataRef();
	}

	comptr_vector(comptr_vector &&other)
		: vec(std::move(other.vec)){
	}

	~comptr_vector(){
		this->ReleaseDataRef();
	}

	comptr_vector &operator=(const comptr_vector &other){
		if (this != &other){
			this->vec = other.vec;

			this->AddDataRef();
		}

		return *this;
	}

	comptr_vector &operator=(comptr_vector &&other){
		if (this != &other){
			this->vec = std::move(other.vec);
		}

		return *this;
	}

	T * const&operator[](size_t idx) const{
		return this->vec[idx];
	}

	T *&operator[](size_t idx){
		return this->vec[idx];
	}

	size_t size() const{
		return this->vec.size();
	}

	pointer data(){
		return this->vec.data();
	}

	const_pointer data() const{
		return this->vec.data();
	}

	iterator begin() {
		return this->vec.begin();
	}

	const_iterator begin() const {
		return this->vec.begin();
	}

	iterator end() {
		return this->vec.end();
	}

	const_iterator end() const {
		return this->vec.end();
	}

	void push_back(const Microsoft::WRL::ComPtr<T> &v){
		auto tmp = v.Get();
		if (tmp){
			static_cast<IUnknown *>(tmp)->AddRef();
		}

		this->vec.push_back(tmp);
	}

	void push_back(T *v){
		if (v){
			static_cast<IUnknown *>(v)->AddRef();
		}
		
		this->vec.push_back(v);
	}

	void reserve(size_t v){
		this->vec.reserve(v);
	}

	void resize(size_t v){
		this->vec.resize(v, nullptr);
	}
private:
	vecType vec;

	void AddDataRef(){
		for (auto &i : this->vec){
			if (i){
				static_cast<IUnknown *>(i)->AddRef();
			}
		}
	}

	void ReleaseDataRef(){
		for (auto &i : this->vec){
			if (i){
				static_cast<IUnknown *>(i)->Release();
				i = nullptr;
			}
		}
	}
};