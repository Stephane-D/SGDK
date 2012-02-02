/**
 * \file joy.h
 * \brief Joypad support
 * \author Stephane Dallongeville
 * \date 08/2011
 *
 * This unit provides methods to read Joypad state.
 */

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


/**
 * \brief
 *      Initialize the controller sub system.<br>
 *
 *      Software and hardware controller port initialization.<br>
 *      Automatically called at SGDK initialization, no need to call it manually.
 */
void JOY_init();

/**
 * \brief
 *      Set the callback function for controller state changed.<br>
 *<br>
 *      SGDK provides facilities to detect state change on controller.<br>
 *      It update controllers state at each V Blank period and fire event if a state change is detected.<br>
 *
 * \param CB
 *      Callback to call when controller(s) state changed.<br>
 *      The function prototype should reply to _joyEventCallback type :<br>
 *      void function(u16 joy, u16 changed, u16 state);<br>
 *<br>
 *      <b>Ex 1</b> : if player 1 just pressed START button you receive :<br>
 *      joy = JOY_1, changed = BUTTON_START, state = BUTTON_START<br>
 *      <b>Ex 2</b> : if player 2 just released the A button you receive :<br>
 *      joy = JOY_2, changed = BUTTON_A, state = 0<br>
 */
void JOY_setEventHandler(_joyEventCallback *CB);
/**
 * \brief
 *      Set peripheral support for the specified port.<br>
 *<br>
 *      By default ports are configured to only support 3 buttons joypad.
 *
 * \param port
 *      Port we want to set support.<br>
 *      <b>PORT_1</b>   = port 1<br>
 *      <b>PORT_2</b>   = port 2
 * \param support
 *      Peripheral support.<br>
 *      <b>JOY_SUPPORT_3BTN</b>         = 3 buttons joypad<br>
 *      <b>JOY_SUPPORT_6BTN</b>         = 6 buttons joypad<br>
 *      <b>JOY_SUPPORT_MOUSE</b>        = sega mouse<br>
 *      <b>JOY_SUPPORT_TEAMPLAY</b>     = Sega TeamPlay or EA 4-Way<br>
 *      <b>JOY_SUPPORT_MENACER</b>      = Sega Menacer<br>
 *      <b>JOY_SUPPORT_ANALOGJOY</b>    = Sega nalog joypad<br>
 *      <b>JOY_SUPPORT_KEYBOARD</b>     = Sega keyboard<br>
 *<br>
 *      Ex : enabled support for 3 and 6 buttons joypad on first port :<br>
 *      JOY_setSupport(PORT_1, JOY_SUPPORT_3BTN | JOY_SUPPORT_6BTN);<br>
 *<br>
 */
void JOY_setSupport(u16 port, u16 support);

/**
 * \brief
 *      Get joypad state.
 *
 * \param joy
 *      Joypad we query state.<br>
 *      <b>JOY_1</b>    = joypad 1<br>
 *      <b>JOY_2</b>    = joypad 2<br>
 *      <b>...  </b>    = ...<br>
 *      <b>JOY_8</b>    = joypad 8 (only possible with 2 teamplayers connected)<br>
 * \return joypad state.<br>
 *      <b>BUTTON_UP</b>    = UP button<br>
 *      <b>BUTTON_DOWN</b>  = DOWN button<br>
 *      <b>BUTTON_LEFT</b>  = LEFT button<br>
 *      <b>BUTTON_RIGHT</b> = RIGHT button<br>
 *      <b>BUTTON_A</b>     = A button<br>
 *      <b>BUTTON_B</b>     = B button<br>
 *      <b>BUTTON_C</b>     = C button<br>
 *      <b>BUTTON_START</b> = START button<br>
 *      <b>BUTTON_X</b>     = X button<br>
 *      <b>BUTTON_Y</b>     = Y button<br>
 *      <b>BUTTON_Z</b>     = Z button<br>
 *      <b>BUTTON_MODE</b>  = MODE button<br>
 *<br>
 *      Ex : Test if button START or A is pressed on joypad 1 :<br>
 *      if (JOY_readJoypad(JOY_1) & (BUTTON_START | BUTTON_A))<br>
 *<br>
 */
u16  JOY_readJoypad(u16 joy);

/**
 * \brief
 *      Wait until a button is pressed on any connected controller.
 */
void JOY_waitPressBtn();
/**
 * \brief
 *      Wait for specified button to be pressed on specified joypad.
 *
 * \param joy
 *      Joypad we want to check (see JOY_readJoypad()).<br>
 *      You can also use JOY_ALL to check on any connected controller.
 * \param btn
 *      button we want to check.<br>
 *      <b>BUTTON_UP</b>    = UP button<br>
 *      <b>BUTTON_DOWN</b>  = DOWN button<br>
 *      <b>BUTTON_LEFT</b>  = LEFT button<br>
 *      <b>BUTTON_RIGHT</b> = RIGHT button<br>
 *      <b>BUTTON_A</b>     = A button<br>
 *      <b>BUTTON_B</b>     = B button<br>
 *      <b>BUTTON_C</b>     = C button<br>
 *      <b>BUTTON_START</b> = START button<br>
 *      <b>BUTTON_X</b>     = X button<br>
 *      <b>BUTTON_Y</b>     = Y button<br>
 *      <b>BUTTON_Z</b>     = Z button<br>
 *      <b>BUTTON_MODE</b>  = MODE button<br>
 *      <b>BUTTON_DIR</b>   = Any of the direction buttons (UP, DOWN, LEFT or RIGHT)<br>
 *      <b>BUTTON_BTN</b>   = Any of the non direction buttons (A, B, C, START, X, Y, Z, MODE)<br>
 *      <b>BUTTON_ALL</b>   = Any of all buttons<br>
 *<br>
 *      Ex : if we want to wait any of direction buttons or button A is pressed on joypad 1 :<br>
 *      JOY_waitJoypad(JOY_1, BUTTON_DIR | BUTTON_A);<br>
 */
void JOY_waitPress(u16 joy, u16 btn);


/**
 * \brief
 *      Manual update joypad state.<br>
 *<br>
 *      By default the library update joypad state on V interrupt process.<br>
 *      Calling this method will force to update joypad state now.
 */
void JOY_update();


#endif // _JOY_H_
