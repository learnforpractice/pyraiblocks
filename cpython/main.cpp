#include <rai/node/rai_.hpp>

static rai::pyrai* g_rai;
extern "C" void register_rai_class(rai::pyrai *rai)
{
	g_rai = rai;
}
namespace rai
{

pyrai* get_pyrai()
{
	return g_rai;
}

}

int main(int argc, char** argv)
{
	printf("%d \n", argc);
	return 0;
}

