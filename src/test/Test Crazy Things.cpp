#include <functional>

void
Foo1(std::function<void(void)> bar1 = []()
	{
	})
{
}
