// NOTE: Won't fold with the token method
// Multi-line function params, should fold
void
Foo1(
	int bar1,
	int bar2)
{
}

struct Foo2
{
	// Member function, should fold
	void Foo3()
	{
	}
};

struct Foo4
{
	// Constructor, should fold
	Foo4()
	{
	}
};

struct Foo5
{
	// Destructor, should fold
	~Foo5()
	{
	}
};

// Operator overload, should fold
struct Foo6
{
	void operator+(const Foo6& other)
	{
	}
};

// Multi-line params with default, should fold
void
Foo7(int bar1
	= 1 + 2)
{
}
