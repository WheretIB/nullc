#pragma once

namespace detail
{
	template<typename T>
	T max(T x, T y)
	{
		return x > y ? x : y;
	}

	template<typename T>
	struct node
	{
		node(): left(NULL), right(NULL), height(1u)
		{
		}

		node* min_tree()
		{
			node* x = this;

			while(x->left)
				x = x->left;

			return x;
		}

		T		key;

		node	*left;
		node	*right;

		int		height;
	};
}

template<typename T>
class Tree
{
public:
	typedef detail::node<T>* iterator;
	typedef detail::node<T> node_type;

	TypedObjectPool<node_type, 1024> pool;

	Tree(): root(NULL)
	{
	}

	void reset()
	{
		pool.Reset();
		root = NULL;
	}

	void clear()
	{
		pool.Reset();
		root = NULL;
	}

	iterator insert(const T& key)
	{
		return root = insert(root, key);
	}

	void erase(const T& key)
	{
		root = erase(root, key);
	}

	iterator find(const T& key)
	{
		node_type *curr = root;

		while(curr)
		{
			if(curr->key == key)
				return iterator(curr);

			curr = key < curr->key ? curr->left : curr->right;
		}

		return iterator(NULL);
	}

	void for_each(void (*it)(T&))
	{
		if(root)
			for_each(root, it);
	}
private:
	int get_height(node_type* n)
	{
		return n ? n->height : 0;
	}

	int get_balance(node_type* n)
	{
		return n ? get_height(n->left) - get_height(n->right) : 0;
	}

	node_type* left_rotate(node_type* x)
	{
		node_type *y = x->right;
		node_type *T2 = y->left;

		y->left = x;
		x->right = T2;

		x->height = detail::max(get_height(x->left), get_height(x->right)) + 1;
		y->height = detail::max(get_height(y->left), get_height(y->right)) + 1;

		return y;
	}

	node_type* right_rotate(node_type* y)
	{
		node_type *x = y->left;
		node_type *T2 = x->right;

		x->right = y;
		y->left = T2;

		y->height = detail::max(get_height(y->left), get_height(y->right)) + 1;
		x->height = detail::max(get_height(x->left), get_height(x->right)) + 1;

		return x;
	}

	node_type* insert(node_type* node, const T& key)
	{
		if(node == NULL)
		{
			node_type *t = pool.Allocate();
			t->key = key;
			return t;
		}

		if(key < node->key)
			node->left = insert(node->left, key);
		else
			node->right = insert(node->right, key);

		node->height = detail::max(get_height(node->left), get_height(node->right)) + 1;

		int balance = get_balance(node);

		if(balance > 1 && key < node->left->key)
			return right_rotate(node);

		if(balance < -1 && key > node->right->key)
			return left_rotate(node);

		if(balance > 1 && key > node->left->key)
		{
			node->left =  left_rotate(node->left);
			return right_rotate(node);
		}

		if(balance < -1 && key < node->right->key)
		{
			node->right = right_rotate(node->right);
			return left_rotate(node);
		}

		return node;
	}

	node_type* erase(node_type* node, const T& key)
	{
		if(node == NULL)
			return node;

		if(key < node->key)
		{
			node->left = erase(node->left, key);
		}else if(key > node->key){
			node->right = erase(node->right, key);
		}else{
			if((node->left == NULL) || (node->right == NULL))
			{
				node_type *temp = node->left ? node->left : node->right;

				if(temp == NULL)
				{
					temp = node;
					node = NULL;
				}else{
					*node = *temp;
				}

				pool.Deallocate(temp);

				if(temp == root)
					root = node;
			}else{
				node_type* temp = node->right->min_tree();

				node->key = temp->key;

				node->right = erase(node->right, temp->key);
			}
		}

		if(node == NULL)
			return node;

		node->height = detail::max(get_height(node->left), get_height(node->right)) + 1;

		int balance = get_balance(node);

		if(balance > 1 && get_balance(node->left) >= 0)
			return right_rotate(node);

		if(balance > 1 && get_balance(node->left) < 0)
		{
			node->left =  left_rotate(node->left);
			return right_rotate(node);
		}

		if(balance < -1 && get_balance(node->right) <= 0)
			return left_rotate(node);

		if(balance < -1 && get_balance(node->right) > 0)
		{
			node->right = right_rotate(node->right);
			return left_rotate(node);
		}

		return node;
	}

	node_type* find(node_type* node, const T& key)
	{
		if(!node)
			return NULL;

		if(node->key == key)
			return node;

		if(key < node->key)
			return find(node->left, key);

		return find(node->right, key);
	}

	void for_each(node_type* node, void (*it)(T&))
	{
		if(node->left)
			for_each(node->left, it);
		it(node->key);
		if(node->right)
			for_each(node->right, it);
	}

	node_type	*root;
};
