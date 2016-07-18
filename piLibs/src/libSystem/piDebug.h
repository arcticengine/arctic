#pragma once

namespace piLibs {

#if defined(_DEBUG)
void piAssert(bool e);
#else
#define piAssert(e) ((void)0)
#endif


} // namespace piLibs