#pragma once

namespace piLibs {

typedef struct
{
    void    *ptr;
    void    *data;
}piShMem;

int  piShMem_Init( piShMem *shmem, const wchar_t *name, long amount );
int  piShMem_Map( piShMem *shmem, const wchar_t *name, long amount );
void piShMem_End( piShMem *shmem );

} // namespace piLibs