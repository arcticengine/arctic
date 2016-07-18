#pragma once

namespace piLibs {

#define KEY_LEFT	0
#define KEY_RIGHT	1
#define KEY_UP		2
#define KEY_DOWN	3
#define KEY_SPACE   4
#define KEY_PGUP	5
#define KEY_PGDOWN	6
#define KEY_ENTER   7
#define KEY_BACK    8
#define KEY_END     9
#define KEY_HOME   10


#define KEY_F1      11
#define KEY_F2      12
#define KEY_F3      13
#define KEY_F4      14
#define KEY_F5      15
#define KEY_F6      16
#define KEY_F7      17
#define KEY_F8      18
#define KEY_F9      19
#define KEY_F10     20
#define KEY_F11     21
#define KEY_F12     22

#define KEY_LSHIFT	    23
#define KEY_RSHIFT	    24
#define KEY_LCONTROL	25
#define KEY_RCONTROL	26
#define KEY_LALT        27
#define KEY_RALT        28
#define KEY_SHIFT	    29
#define KEY_CONTROL	    30
#define KEY_ALT         31

#define KEY_DELETE      129
#define KEY_TAB         130

#define KEY_0		48
#define KEY_1		49
#define KEY_2		50
#define KEY_3		51
#define KEY_4		52
#define KEY_5		53
#define KEY_6		54
#define KEY_7		55
#define KEY_8		56
#define KEY_9		57
/*
#define KEY_A		65
#define KEY_B       66
#define KEY_C       67
#define KEY_D       68
#define KEY_E       69
#define KEY_F       70
#define KEY_G       71
#define KEY_H       72
#define KEY_I       73
#define KEY_J       74
#define KEY_K       75
#define KEY_L       76
#define KEY_M       77
#define KEY_N       78
#define KEY_O       79
#define KEY_P       80
#define KEY_Q		81
#define KEY_R       82
#define KEY_S       83
#define KEY_T       84
#define KEY_U       85
#define KEY_V       86
#define KEY_W       87
#define KEY_X       88
#define KEY_Y       89
#define KEY_Z       90

#define KEY_a		97
#define KEY_b       98
#define KEY_c       99
#define KEY_d       100
#define KEY_e       101
#define KEY_f       102
#define KEY_g       103
#define KEY_h       104
#define KEY_i       105
#define KEY_j       106
#define KEY_k       107
#define KEY_l       108
#define KEY_m       109
#define KEY_n       110
#define KEY_o       111
#define KEY_p       112
#define KEY_q		113
#define KEY_r       114
#define KEY_s       115
#define KEY_t       116
#define KEY_u       117
#define KEY_v       118
#define KEY_w       119
#define KEY_x       120
#define KEY_y       121
#define KEY_z       122
*/

#define TOUCH_NONE				0
#define TOUCH_BEGIN				1
#define TOUCH_PAN				2
#define TOUCH_ZOOM				3
#define TOUCH_ROTATE			4
#define TOUCH_TWOFINGERTAP		5
#define TOUCH_PRESSANDTAP		6
#define TOUCH_END				7

typedef struct
{
    int     state[256];
    int     key[256];
	int     queue[1024];
	int		queueLen;
}piKeyboardInput;

typedef struct
{
    int	    x, y, ox, oy, dx, dy, dz;
    int     lb_isDown;
    int     rb_isDown;
	int     wheel;
}piMouseInput;

typedef struct
{
	int		type;
	int		x;
	int		y;
}piTouchInput;

typedef struct
{
	piKeyboardInput	keyb;
	piMouseInput	mouse;
	piTouchInput	touch;
}piWindowEvents;

typedef void * piWindowMgr;
typedef void * piWindow;


class piWindowMessage
{
public:
    typedef enum
    {
        OK = 1,
        OK_CANCEL = 2,
        YES_NO = 3,
    }MessageType;
    typedef enum
    {
        NONE = 0,
        EXCLAMATION = 1,
        QUESTION = 2,
        STOP = 3,
    }MessageIcon;
    typedef enum
    {
        OK_YES = 1,
        NO_CANCEL = 2,
    }MessageResponse;

    static MessageResponse Prompt(const wchar_t *caption, const wchar_t *message, MessageType type, MessageIcon icon);
};


piWindowMgr piWindowMgr_Init( void );
int         piWindowMgr_MessageLoop( piWindowMgr vme );
void        piWindowMgr_End( piWindowMgr vme );
piWindowEvents *piWindowMgr_getEvents( piWindowMgr vme );

piWindow        piWindow_init( piWindowMgr mgr, const wchar_t *title, int xo, int yo, int xres, int yres, bool full, bool decoration, bool resizable, bool hideCursor );
void            piWindow_end( piWindow me );
void            piWindow_setText( piWindow me, const wchar_t *str );
void           *piWindow_getHandle( piWindow me );
piWindowEvents *piWindow_getEvents( piWindow me );
void            piWindow_dumpBuffer( piWindow vme, void *buffer, int xres, int yres, bool fitwindow );
int             piWindow_getExitReq( piWindow vme );
void            piWIndow_getSize( piWindow vme, int *res );
void            piWindow_hide( piWindow vme );
void            piWindow_show( piWindow vme );

void piWindowEvents_Erase( piWindow vme );
void piWindowEvents_EraseFull( piKeyboardInput *me );
int  piWindowEvents_GetMouse_Dx( piMouseInput *me );
int  piWindowEvents_GetMouse_Dy( piMouseInput *me );
int  piWindowEvents_GetMouse_Dz( piMouseInput *me );
void piWindowEvents_GetMouse_D( piMouseInput *me );

} // namespace piLibs