#ifndef _JOY_H_
#define _JOY_H_


#define PORT_1          0x0000
#define PORT_2          0x0001


#define JOY_1           0x0000
#define JOY_2           0x0001
#define JOY_3           0x0002
#define JOY_4           0x0003
#define JOY_5           0x0004
#define JOY_6           0x0005
#define JOY_7           0x0006
#define JOY_8           0x0007
#define JOY_NUM         0x0008
#define JOY_ALL         0xFFFF


#define BUTTON_UP       0x0001
#define BUTTON_DOWN     0x0002
#define BUTTON_LEFT     0x0004
#define BUTTON_RIGHT    0x0008
#define BUTTON_A        0x0040
#define BUTTON_B        0x0010
#define BUTTON_C        0x0020
#define BUTTON_START    0x0080
#define BUTTON_X        0x0100
#define BUTTON_Y        0x0200
#define BUTTON_Z        0x0400
#define BUTTON_MODE     0x0800

#define BUTTON_DIR      0x000F
#define BUTTON_BTN      0x0FF0
#define BUTTON_ALL      0x0FFF


#define JOY_SUPPORT_3BTN        0x01
#define JOY_SUPPORT_6BTN        0x02
#define JOY_SUPPORT_MOUSE       0x04
#define JOY_SUPPORT_TEAMPLAY    0x08
#define JOY_SUPPORT_MENACER     0x10
#define JOY_SUPPORT_ANALOGJOY   0x20
#define JOY_SUPPORT_KEYBOARD    0x40


typedef void _joyEventCallback(u16 joy, u16 changed, u16 state);


void JOY_init();

void JOY_setEventHandler(_joyEventCallback *CB);
void JOY_setSupport(u16 port, u16 support);

u16  JOY_readJoypad(u16 joy);

void JOY_waitPressBtn();
void JOY_waitPress(u16 joy, u16 btn);

void JOY_update();


#endif // _JOY_H_
