#pragma once

namespace piLibs {


class piTimer
{
public:
    piTimer();
    ~piTimer();

    bool   Init( void );
    void   End(void);
    double GetTime( void );
    int    GetTimeMs( void );
    static void   Sleep( int miliseconds );

    typedef void(*DoAlarmnCallback)(void *data, long time);
    typedef void *Alarm;
    Alarm        CreateAlarm(DoAlarmnCallback func, void *data, int delta);
    void         DestroyAlarm(Alarm me);



private:
    void   *mImplementation;
};

// ---- alarms ----



} // namespace piLibs
