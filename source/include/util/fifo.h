
#ifndef _FIFO_H_
#define _FIFO_H_

struct fifo_elem {
	fifo_elem() { prev = next = nullptr; }
	~fifo_elem() { prev = next = nullptr; }

	fifo_elem* prev;
	fifo_elem* next;
};

template <class T>
class fifo {
public:
	fifo() { _head = _tail = nullptr; }
	~fifo() { _head = _tail = nullptr; }

	void push(T* t) { // 往尾部压入一个元素
		if (t == nullptr)
			return;
		if (_head == nullptr) { // fifo为空
			_head = t;
			_head->prev = nullptr;
			_head->next = nullptr;
			_tail = _head;
		} else { // fifo非空
			_tail->next = t;
			t->prev = _tail;
			t->next = nullptr;
			_tail = t;
		}
	}

	T* pop() { // 从头部取出一个元素
		T* tmp;
		if (_head == _tail) {
			if (_head == nullptr)
				return nullptr; // fifo为空
			else {			 // fifo剩余一个元素
				tmp = _head;
				_head = _tail = nullptr;
				return tmp;
			}
		} else { // fifo中剩余多于一个元素，取出一个
			tmp = _head;
			_head = (T*)_head->next;
			_head->prev = nullptr;
			return tmp;
		}
	}

	int size() const { // 取得fifo的长度
		int sz = 0;
		T* curr = _head;
		if (curr == nullptr)
			return sz;			// fifo为空
		while (curr != _tail) { // fifo非空
			++sz;
			curr = (T*)curr->next;
		}
		return (sz + 1);
	}

protected:
	T* _head;
	T* _tail;
};

#endif // _FIFO_H_
