void
Foo1()
{
	void Foo2(int, int);

	// Multi-line function call, should fold
	Foo2(
		1,
		2
	);
}

void
Foo3()
{
	// Block, should fold
	{
	}
}

// Access specifier, should not fold
struct Foo4
{
private:
	int bar1;
};
