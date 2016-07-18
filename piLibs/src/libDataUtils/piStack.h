#pragma once

namespace piLibs {

class piStack
{
public:
    piStack();
    ~piStack();


    int   Init(int amount, int objectsize);
    void  End(void);

    //void *Push(int num);
    //void  Pop(void *ptr, int num);

    int   IsEmpty(void);
    void  Reset(void);

    void *GetLastData(void);
    int   GetHeadPos(void);
    //void  SetPos(int pos);

    void  Push2( void *obj);
    void *Pop2(void);

private:
    unsigned char *mBuffer;
    int   mAmount;
    //-------
    int mNum;
    int mObjectsize;
};

} // namespace piLibs
