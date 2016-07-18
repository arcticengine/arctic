#pragma once

int  piMainFunc( const wchar_t *path, const wchar_t *cmdline, void *instance );
void piMainCrashHandlerFunc(void * currentThread, void * context);