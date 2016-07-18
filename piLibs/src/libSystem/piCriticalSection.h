#pragma once

namespace piLibs {

typedef struct
{
    void *data;
}piCriticalSection;

bool piCriticalSection_Init(  piCriticalSection *me );
void piCriticalSection_End(  piCriticalSection *me );
void piCriticalSection_Enter( piCriticalSection *me );
void piCriticalSection_Leave( piCriticalSection *me );

} // namespace piLibs