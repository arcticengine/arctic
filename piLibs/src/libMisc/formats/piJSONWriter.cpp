#include "piJSONWriter.h"

namespace piLibs {

typedef struct
{
    int numElemsAdded;    
    bool mIsInline;
}LevelInfo;


piJSONWriter::piJSONWriter()
{
}

piJSONWriter::~piJSONWriter()
{
}



bool piJSONWriter::Begin(piFile *fp)
{
    mFile = fp;
    
    if (!mLevelInfo.Init(1024, sizeof(LevelInfo) ) )
        return false;

    mFile->Prints(L"{\n");

    LevelInfo info = { 0, false };
    mLevelInfo.Push2( &info );

    return true;
}

void piJSONWriter::End(void)
{
    mLevelInfo.Pop2();
    mFile->Prints( L"\n}");

    mLevelInfo.End();
}


void piJSONWriter::MakeTab(void)
{
    int level = mLevelInfo.GetHeadPos();

    const int num = level * 4;

    for (int i = 0; i<num; i++) mTab[i] = L' '; mTab[num] = 0;
}

void piJSONWriter::BeginObject( const wchar_t *name )
{
    LevelInfo *oinfo = (LevelInfo*)mLevelInfo.GetLastData();
    MakeTab();

    wchar_t *pre = (oinfo->numElemsAdded == 0) ? L"" : L",";
    if( name==nullptr )
        mFile->Printf(L"%s\n%s{", pre, mTab);
    else
        mFile->Printf( L"%s\n%s\"%s\" : {", pre, mTab, name);

    LevelInfo info = { 0, false };
    mLevelInfo.Push2(&info);
}

void piJSONWriter::EndObject(void)
{
    mLevelInfo.Pop2();
    LevelInfo *info = (LevelInfo*)mLevelInfo.GetLastData();
    MakeTab();
    mFile->Printf(L"\n%s}", mTab);
    info->numElemsAdded += 1;
}


void piJSONWriter::BeginArray(const wchar_t *name, bool isInline)
{
    LevelInfo *oinfo = (LevelInfo*)mLevelInfo.GetLastData();
    MakeTab();
    mFile->Printf(L"%s\n%s\"%s\" : [", (oinfo->numElemsAdded == 0) ? L"" : L",", mTab, name);

    LevelInfo info = { 0, isInline };
    mLevelInfo.Push2(&info);
}

void piJSONWriter::EndArray(void)
{
    LevelInfo *info = (LevelInfo*)mLevelInfo.GetLastData();
    bool le = info->mIsInline;

    mLevelInfo.Pop2();
    MakeTab();
    if( le )
        mFile->Prints(L"]" );
    else
        mFile->Printf(L"\n%s]", mTab);
    info = (LevelInfo*)mLevelInfo.GetLastData();
    info->numElemsAdded += 1;

}


void piJSONWriter::Add(const wchar_t *name, bool value)
{
    LevelInfo *info = (LevelInfo*)mLevelInfo.GetLastData();
    MakeTab();
    if(name == nullptr)
        mFile->Printf(L"%s\n%s%s", (info->numElemsAdded == 0) ? L"" : L",", mTab, (value) ? L"true" : L"false");
    else
        mFile->Printf(L"%s\n%s\"%s\" : %s", (info->numElemsAdded == 0) ? L"" : L",", mTab, name, (value) ? L"true" : L"false");
    info->numElemsAdded += 1;
}

void piJSONWriter::Add(const wchar_t *name, int value)
{
    LevelInfo *info = (LevelInfo*)mLevelInfo.GetLastData();
    MakeTab();
    if (name == nullptr)
        mFile->Printf(L"%s\n%s%d", (info->numElemsAdded == 0) ? L"" : L",", mTab, value);
    else
        mFile->Printf(L"%s\n%s\"%s\" : %d", (info->numElemsAdded == 0) ? L"" : L",", mTab, name, value);
    info->numElemsAdded += 1;
}

void piJSONWriter::Add(const wchar_t *name, const wchar_t *value)
{
    LevelInfo *info = (LevelInfo*)mLevelInfo.GetLastData();
    MakeTab();
    if (name == nullptr)
        mFile->Printf(L"%s\n%s\"%s\"", (info->numElemsAdded == 0) ? L"" : L",", mTab, value);
    else
        mFile->Printf(L"%s\n%s\"%s\" : \"%s\"", (info->numElemsAdded == 0) ? L"" : L",", mTab, name, value);
    info->numElemsAdded += 1;
}

void piJSONWriter::Add(const wchar_t *name, float value)
{
    LevelInfo *info = (LevelInfo*)mLevelInfo.GetLastData();
    MakeTab();
    const wchar_t *fe = (info->numElemsAdded == 0) ? L"" : L",";

    if( info->mIsInline )
    {
        if (name == nullptr)
            mFile->Printf(L"%s%f", fe, value);
        else
            mFile->Printf(L"%s\"%s\" : %f", fe, name, value);
    }
    else
    {
        if (name == nullptr)
            mFile->Printf(L"%s\n%s%f", fe, mTab, value);
        else
            mFile->Printf(L"%s\n%s\"%s\" : %f", fe, mTab, name, value);
    }

    info->numElemsAdded += 1;
}

void piJSONWriter::Add(const wchar_t *name, double value)
{
    LevelInfo *info = (LevelInfo*)mLevelInfo.GetLastData();
    MakeTab();
    const wchar_t *fe = (info->numElemsAdded == 0) ? L"" : L",";

    if (info->mIsInline)
    {
        if (name == nullptr)
            mFile->Printf(L"%s%lf", fe, value);
        else
            mFile->Printf(L"%s\"%s\" : %lf", fe, name, value);
    }
    else
    {
        if (name == nullptr)
            mFile->Printf(L"%s\n%s%lf", fe, mTab, value);
        else
            mFile->Printf(L"%s\n%s\"%s\" : %lf", fe, mTab, name, value);
    }

    info->numElemsAdded += 1;
}


}