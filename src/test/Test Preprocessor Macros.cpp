// Single line macro, should not fold
#define Foo1

// NOTE: Won't fold with the token method
// Multi-line macro, should fold
#define Foo2 2 \
             1

// Single line function macro, should not fold
#define Foo3(x) x;

// NOTE: Won't fold with the token method
// Multi-line function macro, should fold
#define Foo4(x) \
	if (true)    \
	{            \
		x;        \
	}
