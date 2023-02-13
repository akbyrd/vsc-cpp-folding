void
Foo1()
{
	// Multi-line params function decl, should not fold
	void Foo2(
		int,
		int
	);

	// NOTE: Won't fold with the token method
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

		// Case without braces, should fold
		case 1:
			int x = 1;
			int y = 2;
			break;

		// Case fall through, should fold
		case 2:
			int z = 1;
			int w = 2;
		// Case fall through, should fold
		case 3:
			int a = 1;
			int b = 2;
			break;
	}

	switch (0)
	{
		// Default case, should fold
		// Case block, should not fold
		default:
		{
			break;
		}
	}

	switch (0)
	{
		// Default case without braces, should fold
		default:
			int x = 1;
			int y = 2;
			break;
	}

	switch (0)
	{
		// Default case fall through, should fold
		default:
			int z = 1;
			int w = 2;
		case 3:
			int a = 1;
			int b = 2;
			break;
	}

	switch (0)
	{
		case 2:
			int z = 1;
			int w = 2;
		// Default case fall through, should fold
		default:
			int a = 1;
			int b = 2;
			break;
	}
}
