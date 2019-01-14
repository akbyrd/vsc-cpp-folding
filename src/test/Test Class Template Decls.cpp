// Template class, should fold
template <class T, class U>
class Foo1
{
	T bar1;
	U bar2;
};

// Template class partial specialization, should fold
template<class T>
class Foo1<T, int>
{
};
