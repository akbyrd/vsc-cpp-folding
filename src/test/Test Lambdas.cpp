void Foo1()
{
	// Single line lambda decl, should not fold
	auto lambda1 = [](){};

	// Single line lambda call, should not fold
	lambda1();
}

void Foo2()
{
	// Multi-line lambda decl, should fold
	auto lambda1 = []() {
	};

	// NOTE: Won't fold with the token method
	// Mulit-line lambda call, should fold
	lambda1(
	);
}
