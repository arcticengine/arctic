#pragma once

#include "../../libSystem/piFile.h"
#include "../../libDataUtils/piStack.h"

namespace piLibs {

class piJSONWriter
{
public:
    piJSONWriter();
    ~piJSONWriter();

    bool Begin(piFile *fp);
    void End(void);
    void BeginObject(const wchar_t *name);
    void EndObject(void);

    void BeginArray(const wchar_t *name, bool isInline);
    void EndArray(void);

    void Add(const wchar_t *name, bool value);
    void Add(const wchar_t *name, int value);
    void Add(const wchar_t *name, const wchar_t *value);
    void Add(const wchar_t *name, float value);
    void Add(const wchar_t *name, double value);

private:
    void MakeTab(void);

private:
    piFile *mFile;
    //int    mLevel;
    wchar_t mTab[256];
    piStack mLevelInfo;
};

} // namespace piLibs