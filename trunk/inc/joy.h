/**
 *  \file joy.h
 *  \brief General controller support.
 *  \author Chilly Willy & Stephane Dallongeville
 *  \date 05/2012
 *
 * This unit provides methods to read controller state.<br/>
 *<br/>
 * Here is the list of supported controller device:<br/>
 * - 3 buttons joypad<br/>
 * - 6 buttons joypad<br/>
 * - Sega Mouse<br/>
 * - team player adapter<br/>
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
#define BUTTON_X        0x0400
#define BUTTON_Y        0x0200
#define BUTTON_Z        0x0100
#define BUTTON_MODE     0x0800

#define BUTTON_LMB      0x0040
#define BUTTON_MMB      0x0010
#define BUTTON_RMB      0x0020

#define BUTTON_DIR      0x000F
#define BUTTON_BTN      0x0FF0
#define BUTTON_ALL      0x0FFF

#define JOY_TYPE_PAD3           0x00
#define JOY_TYPE_PAD6           0x01
#define JOY_TYPE_MOUSE          0x02
#define JOY_TYPE_TRACKBALL      0x03
#define JOY_TYPE_MENACER        0x04
#define JOY_TYPE_JUSTIFIER      0x05
#define JOY_TYPE_UNKNOWN        0x0F

#define PORT_TYPE_MENACER       0x00
#define PORT_TYPE_JUSTIFIER     0x01
#define PORT_TYPE_MOUSE         0x03
#define PORT_TYPE_TEAMPLAYER    0x07
#define PORT_TYPE_PAD           0x0D
#define PORT_TYPE_UKNOWN        0x0F
#define PORT_TYPE_EA4WAYPLAY    0x10

#define JOY_SUPPORT_OFF             0x00
#define JOY_SUPPORT_3BTN            0x01
#define JOY_SUPPORT_6BTN            0x02
#define JOY_SUPPORT_MOUSE           0x03
#define JOY_SUPPORT_TRACKBALL       0x04
#define JOY_SUPPORT_TEAMPLAYER      0x05
#define JOY_SUPPORT_EA4WAYPLAY      0x06
#define JOY_SUPPORT_MENACER         0x07
#define JOY_SUPPORT_JUSTIFIER_BLUE  0x08
#define JOY_SUPPORT_JUSTIFIER_BOTH  0x09
#define JOY_SUPPORT_ANALOGJOY       0x0A
#define JOY_SUPPORT_KEYBOARD        0x0B


typedef void _joyEventCallback(u16 joy, u16 changed, u16 state);


/**
 *  \brief
 *      Initialize the controller sub system.<br>
 *
 *      Software and hardware controller port initialization.<br>
 *      Automatically called at SGDK initialization, no need to call it manually.
 */
void JOY_init();

/**
 *  \brief
 *      Set the callback function for controller state changed.<br>
 *<br>
 *      SGDK provides facilities to detect state change on controller.<br>
 *      It update controllers state at each V Blank period and fire event if a state change is detected.<br>
 *
 *  \param CB
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
 *  \brief
 *      Set peripheral support for the specified port.<br>
 *<br>
 *      By default ports are configured to only enable support for joypads, unless<br>
 *      a pad is not detected. In that case, a multitap or mouse is enabled if<br>
 *      present.<br>
 *<br>
 *  \param port
 *      Port we want to set support.<br>
 *      <b>PORT_1</b>   = port 1<br>
 *      <b>PORT_2</b>   = port 2<br>
 *  \param support
 *      Peripheral support.<br>
 *      <b>JOY_SUPPORT_OFF</b>            = No peripheral support<br>
 *      <b>JOY_SUPPORT_3BTN</b>           = 3 button joypad<br>
 *      <b>JOY_SUPPORT_6BTN</b>           = 6 button joypad<br>
 *      <b>JOY_SUPPORT_TRACKBALL</b>      = Sega Sports Pad (SMS trackball)<br>
 *      <b>JOY_SUPPORT_MOUSE</b>          = Sega MegaMouse<br>
 *      <b>JOY_SUPPORT_TEAMPLAYER</b>     = Sega TeamPlayer<br>
 *      <b>JOY_SUPPORT_EA4WAYPLAY</b>     = EA 4-Way Play<br>
 *      <b>JOY_SUPPORT_MENACER</b>        = Sega Menacer<br>
 *      <b>JOY_SUPPORT_JUSTIFIER_BLUE</b> = Konami Justifier (blue gun only)<br>
 *      <b>JOY_SUPPORT_JUSTIFIER_BOTH</b> = Konami Justifier (both guns)<br>
 *      <b>JOY_SUPPORT_ANALOGJOY</b>      = Sega analog joypad (not yet supported)<br>
 *      <b>JOY_SUPPORT_KEYBOARD</b>       = Sega keyboard (not yet supported)<br>
 *<br>
 *      Ex : enable support for MegaMouse on second port :<br>
 *      JOY_setSupport(PORT_2, JOY_SUPPORT_MOUSE);<br>
 *<br>
 */
void JOY_setSupport(u16 port, u16 support);

/**
 *  \brief
 *      Get peripheral type for the specified port.<br>
 *<br>
 *      The peripheral type for each port is automatically detected during JOY_init().<br>
 *      This function returns that type to help decide how the port support should be set.<br>
 *      Types greater than 15 are not derived via Sega's controller ID method.<br>
 *<br>
 *  \param port
 *      Port we want to get the peripheral type.<br>
 *      <b>PORT_1</b>   = port 1<br>
 *      <b>PORT_2</b>   = port 2<br>
 *  \return type
 *      Peripheral type.<br>
 *      <b>PORT_TYPE_MENACER</b>        = Sega Menacer<br>
 *      <b>PORT_TYPE_JUSTIFIER</b>      = Konami Justifier<br>
 *      <b>PORT_TYPE_MOUSE</b>          = Sega MegaMouse<br>
 *      <b>PORT_TYPE_TEAMPLAYER</b>     = Sega TeamPlayer<br>
 *      <b>PORT_TYPE_PAD</b>            = Sega joypad<br>
 *      <b>PORT_TYPE_UNKNOWN</b>        = unidentified or no peripheral<br>
 *      <b>PORT_TYPE_EA4WAYPLAY</b>     = EA 4-Way Play<br>
 *<br>
 *      Ex : get peripheral type in port 1 :<br>
 *      type = JOY_getPortType(PORT_1);<br>
 *<br>
 */
u8 JOY_getPortType(u16 port);

/**
 *  \brief
 *      Get joypad peripheral type connected to the specified joypad port.<br>
 *      Prefer this method over JOY_getPortType(..) when you need to get information<br>
 *      about peripheral connected to multi joypad adapter (as the Sega TeamPlayer).
 *
 *  \param joy
 *      Joypad port we query type.<br>
 *      <b>JOY_1</b>    = joypad 1<br>
 *      <b>JOY_2</b>    = joypad 2<br>
 *      <b>...  </b>    = ...<br>
 *      <b>JOY_8</b>    = joypad 8 (only possible with 2 TeamPlayer connected)<br>
 *  \return joypad peripheral type.<br>
 *      <b>JOY_TYPE_PAD3</b>        = 3 buttons joypad<br>
 *      <b>JOY_TYPE_PAD6</b>        = 6 buttons joypad<br>
 *      <b>JOY_TYPE_MOUSE</b>       = Sega Mouse<br>
 *      <b>JOY_TYPE_TRACKBALL</b>   = Sega trackball<br>
 *      <b>JOY_TYPE_MENACER</b>     = Sega Menacer gun<br>
 *      <b>JOY_TYPE_JUSTIFIER</b>   = Sega Justifier gun<br>
 *      <b>JOY_TYPE_UNKNOWN</b>     = Unknow adaptater or not connected<br>
 */
u8 JOY_getJoypadType(u16 joy);

/**
 *  \brief
 *      Get joypad state.
 *
 *  \param joy
 *      Joypad we query state.<br>
 *      <b>JOY_1</b>    = joypad 1<br>
 *      <b>JOY_2</b>    = joypad 2<br>
 *      <b>...  </b>    = ...<br>
 *      <b>JOY_8</b>    = joypad 8 (only possible with 2 teamplayers connected)<br>
 *      <b>JOY_ALL</b>  = joypad 1 | joypad 2 | ... | joypad 8<br>
 *  \return joypad state.<br>
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
 *      <b>BUTTON_LMB</b>   = Alias for A button for mouse<br>
 *      <b>BUTTON_MMB</b>   = Alias for B button for mouse<br>
 *      <b>BUTTON_RMC</b>   = Alias for C button for mouse<br>
 *<br>
 *      Ex : Test if button START or A is pressed on joypad 1 :<br>
 *      if (JOY_readJoypad(JOY_1) & (BUTTON_START | BUTTON_A))<br>
 *<br>
 */
u16  JOY_readJoypad(u16 joy);

/**
 *  \brief
 *      Get joypad X axis.
 *
 *  \param joy
 *      Joypad we query state.<br>
 *      <b>JOY_1</b>    = joypad 1<br>
 *      <b>JOY_2</b>    = joypad 2<br>
 *      <b>...  </b>    = ...<br>
 *      <b>JOY_8</b>    = joypad 8 (only possible with 2 teamplayers connected)<br>
 *  \return joypad X axis.<br>
 *      A mouse returns signed axis data. The change in this value indicates movement -<br>
 *      to the right for positive changes, or left for negative changes.<br>
 *<br>
 *      A light gun returns the unsigned screen X coordinate. This is not calibrated;<br>
 *      Calibration is left to the game to handle. The value is -1 if the gun is not<br>
 *      pointed at the screen, or the screen is too dim to detect.<br>
 *<br>
 *      Ex : Get X axis of pad 2 :<br>
 *      countX = JOY_readJoypadX(JOY_2);<br>
 *<br>
 */
u16  JOY_readJoypadX(u16 joy);

/**
 *  \brief
 *      Get joypad Y axis.
 *
 *  \param joy
 *      Joypad we query state.<br>
 *      <b>JOY_1</b>    = joypad 1<br>
 *      <b>JOY_2</b>    = joypad 2<br>
 *      <b>...  </b>    = ...<br>
 *      <b>JOY_8</b>    = joypad 8 (only possible with 2 teamplayers connected)<br>
 *  \return joypad Y axis.<br>
 *      A mouse returns signed axis data. The change in this value indicates movement -<br>
 *      upwards for positive changes, or downwards for negative changes.<br>
 *<br>
 *      A light gun returns the unsigned screen Y coordinate. This is not calibrated;<br>
 *      Calibration is left to the game to handle. The value is -1 if the gun is not<br>
 *      pointed at the screen, or the screen is too dim to detect.<br>
 *<br>
 *      Ex : Get Y axis of pad 2 :<br>
 *      countY = JOY_readJoypadY(JOY_2);<br>
 *<br>
 */
u16  JOY_readJoypadY(u16 joy);

/**
 *  \brief
 *      Wait until a button is pressed on any connected controller.
 */
void JOY_waitPressBtn();
/**
 *  \brief
 *      Wait the specified amount of time or until a button is pressed on any connected controller.
 */
u16 JOY_waitPressBtnTime(u16 ms);
/**
 *  \brief
 *      Wait for specified button to be pressed on specified joypad.
 *
 *  \param joy
 *      Joypad we want to check state (see JOY_readJoypad()).<br>
 *      You can also use JOY_ALL to check on any connected controller.
 *  \param btn
 *      button(s) we want to check state.<br>
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
 *      <b>BUTTON_LMB</b>   = Alias for A button for mouse<br>
 *      <b>BUTTON_MMB</b>   = Alias for B button for mouse<br>
 *      <b>BUTTON_RMC</b>   = Alias for C button for mouse<br>
 *  \return
 *      The button actually pressed or FALSE if none of specified button has be pressed in the given time.
 *<br>
 *      Ex: if we want to wait any of direction buttons or button A is pressed on joypad 1 :<br>
 *      pressed = JOY_waitJoypad(JOY_1, BUTTON_DIR | BUTTON_A);<br>
 */
u16 JOY_waitPress(u16 joy, u16 btn);
/**
 *  \brief
 *      Wait for specified button(s) to be pressed on specified joypad.
 *
 *  \param joy
 *      Joypad we want to check state (see JOY_readJoypad()).<br>
 *      You can also use JOY_ALL to check on any connected controller.
 *  \param btn
 *      button(s) we want to check state.<br>
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
 *      <b>BUTTON_LMB</b>   = Alias for A button for mouse<br>
 *      <b>BUTTON_MMB</b>   = Alias for B button for mouse<br>
 *      <b>BUTTON_RMC</b>   = Alias for C button for mouse<br>
 *  \param ms
 *      maximum time in ms to wait for the button press action (0 means wait infinitely).<br/>
 *  \return
 *      The button actually pressed or FALSE if none of specified button has be pressed in the given time.
 *<br>
 *      Ex: if we want to wait a maximum of 5 secondes for any of direction buttons<br/>
 *      or button A to be pressed on joypad 1:<br>
 *      pressed = JOY_waitJoypad(JOY_1, BUTTON_DIR | BUTTON_A, 5000);<br>
 */
u16 JOY_waitPressTime(u16 joy, u16 btn, u16 ms);

/**
 *  \brief
 *      Manual update joypad state.<br>
 *<br>
 *      By default the library update joypad state on V interrupt process.<br>
 *      Calling this method will force to update joypad state now.<br>
 */
void JOY_update();


#endif // _JOY_H_
