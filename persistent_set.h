#pragma once
#include <iostream>

template <typename T>
struct shared_ptr {
	shared_ptr() noexcept : cnt(nullptr), ptr(nullptr) {}
	explicit shared_ptr(T* p) noexcept : cnt(new size_t(1)), ptr(p) {}
	shared_ptr(shared_ptr const& p) noexcept {
		ptr = p.ptr;
		cnt = p.cnt;
		if (cnt) {
			++*cnt;
		}
	}
	~shared_ptr() noexcept {
		if (ptr) {
			--*cnt;
			if (*cnt == 0) {
				delete cnt;
				delete ptr;
			}
		}
	}
	T* get() const noexcept {
		return ptr;
	}
	T* operator->() noexcept {
		return ptr;
	}
	T* operator->() const noexcept {
		return ptr;
	}
	T& operator*() noexcept {
		return *ptr;
	}
	void swap(shared_ptr& rhs) noexcept {
		std::swap(cnt, rhs.cnt);
		std::swap(ptr, rhs.ptr);
	}
	shared_ptr& operator=(shared_ptr const& rhs) noexcept {
		shared_ptr copy(rhs);
		swap(copy);
		return *this;
	}
private:
	size_t* cnt;
	T* ptr;
};

template<typename T>
struct linked_ptr {
	linked_ptr() noexcept : ptr(nullptr), prev(nullptr), next(nullptr) {}
	linked_ptr(T* p) noexcept : ptr(p), prev(nullptr), next(nullptr) {}
	linked_ptr(linked_ptr const& p) noexcept : ptr(p.ptr) {
		prev = &p;
		next = p.next;
		if (p.next) {
			p.next->prev = this;
		}
		p.next = this;
	}
	~linked_ptr() noexcept {
		if (!prev && !next) {
			delete ptr;
		}
		if (prev) {
			prev->next = next;
		}
		if (next) {
			next->prev = prev;
		}
	}
	T* get() const noexcept {
		return ptr;
	}
	T* operator->() noexcept {
		return ptr;
	}
	T* operator->() const noexcept {
		return ptr;
	}
	T& operator*() noexcept {
		return *ptr;
	}
	void swap(linked_ptr& rhs) noexcept {
		std::swap(ptr, rhs.ptr);

		//std::swap(prev->next, rhs.prev->next);
		if (prev) {
			prev->next = &rhs;
		}
		if (rhs.prev) {
			rhs.prev->next = this;
		}

		//std::swap(next->prev, rhs.next->prev);
		if (next) {
			next->prev = &rhs;
		}
		if (rhs.next) {
			rhs.next->prev = this;
		}

		std::swap(prev, rhs.prev);
		std::swap(next, rhs.next);
	}
	linked_ptr& operator=(linked_ptr const& rhs) noexcept {
		linked_ptr copy(rhs);
		swap(copy);
		return *this;
	}
private:
	T* ptr;
	mutable linked_ptr const *prev;
	mutable linked_ptr const *next;
};

template <typename T, template <typename> class smart_ptr = shared_ptr>
struct persistent_set {
private:
	struct node_base {
		smart_ptr<node_base> left;
		virtual ~node_base() {}
	};

	struct node : node_base {
		smart_ptr<node_base> right;
		smart_ptr<T> data;
		node(smart_ptr<node_base> left, smart_ptr<node_base> right, smart_ptr<T> data) noexcept : right(right), data(data) {
			this->left = left;
		}
		explicit node(T const& obj) : data(new T(obj)) {}
		explicit node(T&& obj) : data(new T(std::move(obj))) {}
	};
public:
	struct iterator {
		node_base *p, *fake_node;
		iterator(node_base* p, node_base* fake_node) : p(p), fake_node(fake_node) {}

		iterator& operator++();
		iterator& operator--();
		iterator operator++(int);
		iterator operator--(int);
		T const& operator*() const noexcept {
			return *static_cast<node*>(p)->data;
		}

		friend bool operator==(iterator const& a, iterator const& b) noexcept {
			return a.p == b.p;
		}

		friend bool operator!=(iterator const& a, iterator const& b) noexcept {
			return a.p != b.p;
		}
	};

	persistent_set() noexcept : fake_node(new node_base()) {}

	persistent_set(persistent_set const& other) noexcept : fake_node(new node_base()) {
		fake_node.get()->left = other.fake_node.get()->left;
	}

	void swap(persistent_set& rhs) noexcept {
		fake_node.swap(rhs.fake_node);
	}

	persistent_set& operator=(persistent_set const& rhs) noexcept {
		persistent_set copy(rhs);
		swap(copy);
		return *this;
	}

	iterator find(T const& obj) const;
	std::pair<iterator, bool> insert(T new_data);
	void erase(iterator const& removable_node_iterator);

	iterator begin() const;
	iterator end() const noexcept;

private:
	std::pair<node_base*, node_base*> insert(smart_ptr<node_base>& cur_node, T& new_data);
	smart_ptr<node_base> erase(smart_ptr<node_base> cur_node, node_base* removable_node);

	smart_ptr<node_base> fake_node;
};

template <typename T, template <typename> class smart_ptr>
typename persistent_set<T, smart_ptr>::iterator& persistent_set<T, smart_ptr>::iterator::operator++() {
	if (static_cast<node*>(p)->right.get()) {
		p = static_cast<node*>(p)->right.get();
		while (p->left.get()) {
			p = p->left.get();
		}
		return *this;
	}
	node_base* res = fake_node;
	node_base* cur = res->left.get();
	while (cur != p) {
		if (*static_cast<node*>(cur)->data < *static_cast<node*>(p)->data) {
			cur = static_cast<node*>(cur)->right.get();
		} else {
			res = cur;
			cur = cur->left.get();
		}
	}
	p = res;
	return *this;
}

template <typename T, template <typename> class smart_ptr>
typename persistent_set<T, smart_ptr>::iterator& persistent_set<T, smart_ptr>::iterator::operator--() {
	if (p->left.get()) {
		p = p->left.get();
		while (static_cast<node*>(p)->right.get()) {
			p = static_cast<node*>(p)->right.get();
		}
		return *this;
	}
	node_base* res = fake_node->left.get();
	node_base* cur = res;
	while (cur != p) {
		if (*static_cast<node*>(cur)->data < *static_cast<node*>(p)->data) {
			res = cur;
			cur = static_cast<node*>(cur)->right.get();
		} else {
			cur = cur->left.get();
		}
	}
	p = res;
	return *this;
}

template <typename T, template <typename> class smart_ptr>
typename persistent_set<T, smart_ptr>::iterator persistent_set<T, smart_ptr>::iterator::operator++(int) {
	iterator res = *this;
	++*this;
	return res;
}

template <typename T, template <typename> class smart_ptr>
typename persistent_set<T, smart_ptr>::iterator persistent_set<T, smart_ptr>::iterator::operator--(int) {
	iterator res = *this;
	--*this;
	return res;
}

template <typename T, template <typename> class smart_ptr>
typename persistent_set<T, smart_ptr>::iterator persistent_set<T, smart_ptr>::find(T const& obj) const {
	node_base* cur = fake_node->left.get();
	while (cur) {
		if (*static_cast<node*>(cur)->data == obj) {
			return iterator(cur, fake_node.get());
		}
		if (*static_cast<node*>(cur)->data < obj) {
			cur = static_cast<node*>(cur)->right.get();
		} else {
			cur = cur->left.get();
		}
	}
	return end();
}

template <typename T, template <typename> class smart_ptr>
std::pair<typename persistent_set<T, smart_ptr>::iterator, bool> persistent_set<T, smart_ptr>::insert(T new_data) {
	std::pair<node_base*, node_base* > res = insert(fake_node.get()->left, new_data);
	if (res.second == nullptr) {
		return std::make_pair(iterator(res.first, fake_node.get()), false);
	}
	smart_ptr<node_base> p(res.second);
	fake_node.get()->left = p;
	return std::make_pair(iterator(res.first, fake_node.get()), true);
}

template <typename T, template <typename> class smart_ptr>
std::pair<typename persistent_set<T, smart_ptr>::node_base*, typename persistent_set<T, smart_ptr>::node_base*> persistent_set<T, smart_ptr>::insert(smart_ptr<node_base>& cur_node, T& new_data) {
	if (cur_node.get() == nullptr) {
		node_base* new_node = new node(std::move(new_data));
		return std::make_pair(new_node, new_node);
	}
	if (*static_cast<node*>(cur_node.get())->data == new_data) {
		return std::make_pair(cur_node.get(), nullptr);
	}
	if (*static_cast<node*>(cur_node.get())->data < new_data) {
		auto res = insert(static_cast<node*>(cur_node.get())->right, new_data);
		if (res.second == nullptr) {
			return std::make_pair(res.first, nullptr);
		}
		return std::make_pair(res.first,
			new node(cur_node.get()->left, smart_ptr<node_base>(res.second), static_cast<node*>(cur_node.get())->data));
	}
	auto res = insert(cur_node.get()->left, new_data);
	if (res.second == nullptr) {
		return std::make_pair(res.first, nullptr);
	}
	return std::make_pair(res.first,
		new node(smart_ptr<node_base>(res.second), static_cast<node*>(cur_node.get())->right, static_cast<node*>(cur_node.get())->data));
}

template <typename T, template <typename> class smart_ptr>
void persistent_set<T, smart_ptr>::erase(iterator const& removable_node_iterator) {
	smart_ptr<node_base> p = erase(fake_node.get()->left, removable_node_iterator.p);
	fake_node->left = p;
}

template <typename T, template <typename> class smart_ptr>
smart_ptr<typename persistent_set<T, smart_ptr>::node_base> persistent_set<T, smart_ptr>::erase(smart_ptr<node_base> cur_node, node_base* removable_node) {
	if (*static_cast<node*>(cur_node.get())->data.get() < *static_cast<node*>(removable_node)->data) {
		return smart_ptr<node_base>(new node(cur_node->left,
			erase(static_cast<node*>(cur_node.get())->right, removable_node),
			static_cast<node*>(cur_node.get())->data));
	}
	if (*static_cast<node*>(removable_node)->data < *static_cast<node*>(cur_node.get())->data.get()) {
		return smart_ptr<node_base>(new node(erase(cur_node->left, removable_node),
			static_cast<node*>(cur_node.get())->right,
			static_cast<node*>(cur_node.get())->data));
	}
	if (cur_node->left.get() == nullptr) {
		return static_cast<node*>(cur_node.get())->right;
	}
	if (static_cast<node*>(cur_node.get())->right.get() == nullptr) {
		return cur_node->left;
	}
	node_base* v = static_cast<node*>(cur_node.get())->right.get();
	while (v->left.get()) {
		v = v->left.get();
	}
	return smart_ptr<node_base>(new node(cur_node->left,
		smart_ptr<node_base>(erase(static_cast<node*>(cur_node.get())->right, v)),
		static_cast<node*>(v)->data));
}

template <typename T, template <typename> class smart_ptr>
typename persistent_set<T, smart_ptr>::iterator persistent_set<T, smart_ptr>::begin() const {
	node_base* v = fake_node.get();
	while (v->left.get()) {
		v = v->left.get();
	}
	return iterator(v, fake_node.get());
}

template <typename T, template <typename> class smart_ptr>
typename persistent_set<T, smart_ptr>::iterator persistent_set<T, smart_ptr>::end() const noexcept {
	return iterator(fake_node.get(), fake_node.get());
}
