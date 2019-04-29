// Template function, should fold
template<class T>
void
Foo1(T bar1)
{
}

// Multi-line template params, should not fold
template<
	class T1,
	class T2
>
void
Foo2(T1 bar1, T2 bar2)
{
}
