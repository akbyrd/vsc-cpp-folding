// Struct, should fold
struct Foo1
{
};

// Union, should fold
union Foo2
{
};

// Class, should fold
class Foo3
{
};

// Enum, should fold
enum Foo4
{
	None,
};

// Namespace, should fold
namespace Foo5
{
}
