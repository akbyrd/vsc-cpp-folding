void
Foo1()
{
	// Multi-line params function decl, should not fold
	void Foo2(
		int,
		int
	);

	// Multi-line params function call, should fold
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

void
Foo5()
{
	// If, should fold
	// If block, should not fold
	if (true)
	{
	}
	// Else, should fold
	// Else block, should not fold
	else
	{
	}
}

// For loop, should fold
// For loop block, should not fold
void
Foo5()
{
	for (;;)
	{
	}
}

// While loop, should fold
// While loop block, should not fold
void
Foo6()
{
	while (true)
	{
	}
}

// Do-while loop, should fold
// Do-while loop block, should not fold
void
Foo7()
{
	do
	{
	} while (true);
}

void
Foo8()
{
	// Switch, should fold
	// Switch block, should not fold
	switch (0)
	{
		// Case, should fold
		// Case block, should not fold
		case 0:
		{
			break;
		}
	}
}
