#pragma once

namespace piLibs {


typedef struct
{
    void *data;
}piSpinLock;

bool piSpinLock_Init( piSpinLock *me, unsigned int count );
void piSpinLock_Free( piSpinLock *me );
void piSpinLock_Enter( piSpinLock *me );
void piSpinLock_Leave( piSpinLock *me );

} // namespace piLibs