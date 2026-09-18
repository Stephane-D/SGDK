#include "unity.h"
#include "genesis.h"
#include "fff.h"

#define LOOP_COUNT 100

DEFINE_FFF_GLOBALS;

//FAKE_VALUE_FUNC0(bool, SYS_doVBlankProcess);
FAKE_VOID_FUNC0(SYS_disableInts);
FAKE_VOID_FUNC0(SYS_enableInts);
FAKE_VOID_FUNC0(SPR_init);
FAKE_VOID_FUNC0(SPR_update);
FAKE_VOID_FUNC1(XGM_startPlay, const u8 *);
FAKE_VALUE_FUNC4(Sprite*, SPR_addSprite, const SpriteDefinition *, s16,  s16,  u16);
FAKE_VOID_FUNC1(JOY_setEventHandler, JoyEventCallback *);

static u16 vblankProcessCalls;
static JoyEventCallback *registeredJoyCallback;

static bool testLoopShouldContinue(void)
{
    return vblankProcessCalls <= LOOP_COUNT;
}

static bool testSysDoVBlankProcess(void)
{
    vblankProcessCalls++;
    if (vblankProcessCalls == 50 && registeredJoyCallback != NULL) {
        // Simulación de evento de mando en el ciclo 50 (Pulsar botón START en JOY_1)
        registeredJoyCallback(JOY_1, BUTTON_START, BUTTON_START);
    }
    return TRUE;
}

static void testJoySetEventHandler(JoyEventCallback *cb)
{
    registeredJoyCallback = cb;
}
const SpriteDefinition spr_donut = { 0 };
const u8 mus_actraiser[70656] = { 0 };


/* Replace hardware operations so the application main can run once on the host. */
#define main applicationMain
#define MAIN_LOOP_CONDITION testLoopShouldContinue()
#define SYS_doVBlankProcess() testSysDoVBlankProcess()
#define JOY_setEventHandler(cb) testJoySetEventHandler(cb)
#define kprintf(...) printf(__VA_ARGS__)
#include "../../src/main.c"
#undef JOY_setEventHandler
#undef SYS_doVBlankProcess
#undef kprintf
#undef MAIN_LOOP_CONDITION
#undef main

void setUp(void)
{    
    
    //RESET_FAKE(SYS_doVBlankProcess);
    RESET_FAKE(SYS_disableInts);
    RESET_FAKE(SYS_enableInts);
    RESET_FAKE(SPR_init);
    RESET_FAKE(SPR_update);
    RESET_FAKE(XGM_startPlay);
    RESET_FAKE(SPR_addSprite);
    //RESET_FAKE(JOY_setEventHandler);
    vblankProcessCalls = 0;
    registeredJoyCallback = NULL;
}

void tearDown(void)
{
}

void test_main_calls_sys_do_vblank_process(void)
{
    applicationMain(FALSE);

    TEST_ASSERT_GREATER_OR_EQUAL_UINT16(LOOP_COUNT, vblankProcessCalls);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_main_calls_sys_do_vblank_process);
    return UNITY_END();
}
