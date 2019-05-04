#if false
	void
	Foo1()
	{
	}
#else
	void
	Foo2()
	{
	}
#endif


// TODO: Find something sane to do here
void Foo3()
{
	if (false)
	{

	#if false
	}
	#else
	}
	#endif
}
