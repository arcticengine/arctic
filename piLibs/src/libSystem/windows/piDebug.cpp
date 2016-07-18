
namespace piLibs {

void piAssert(bool e)
{
    if (!e)  __debugbreak();
}


}