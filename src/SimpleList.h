#ifndef _BGA_SIMPLELIST_H_
#define _BGA_SIMPLELIST_H_

#include <vector>

template<class T>
class SimpleList {
	std::vector<T> list;
	int _size;

public:
	SimpleList() {
		_size=0;
	}
	~SimpleList() {
	}
	void clear() {
		_size=0;
	}
	void clear_hard() {
		_size=0;
		list.clear();
	}
	int size() const {
		return _size;
	}
	void push_back(T item) {
		if(_size==list.size()) {
			list.push_back(item);
		}
		else {
			list[_size]=item;
		}
		_size++;
	}
	T operator[](int i) const {
		return list[i];
	}
	bool contains(const T& item) const {
		for(int i=0;i<_size;i++) {
			if(list[i]==item) {
				return true;
			}
		}
		return false;
	}
};

#endif
