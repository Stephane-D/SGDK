#include "unity.h"
#include "genesis.h"

static u16 vblankProcessCalls;
static JoyEventCallback *registeredJoyCallback;

static bool testLoopShouldContinue(void)
{
    return vblankProcessCalls <= 100;
}

static void testJoySetEventHandler(JoyEventCallback *cb)
{
    registeredJoyCallback = cb;
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

/* Replace hardware operations so the application main can run once on the host. */
#define main applicationMain
#define TRUE testLoopShouldContinue()
#define SYS_disableInts() ((void)0)
#define SYS_enableInts() ((void)0)
#define SPR_init() ((void)0)
#define JOY_setEventHandler(cb) testJoySetEventHandler(cb)
#define XGM_startPlay(song) ((void)0)
#define SPR_addSprite(...) ((void)0)
#define SPR_update() ((void)0)
#define SYS_doVBlankProcess() testSysDoVBlankProcess()
#define kprintf(...) ((void)0)
#include "../../src/main.c"
#undef SYS_doVBlankProcess
#undef kprintf
#undef SPR_update
#undef SPR_addSprite
#undef XGM_startPlay
#undef JOY_setEventHandler
#undef SPR_init
#undef SYS_enableInts
#undef SYS_disableInts
#undef TRUE
#undef main

void setUp(void)
{
    vblankProcessCalls = 0;
    registeredJoyCallback = NULL;
}

void tearDown(void)
{
}

void test_main_calls_sys_do_vblank_process(void)
{
    applicationMain(FALSE);

    TEST_ASSERT_GREATER_OR_EQUAL_UINT16(100, vblankProcessCalls);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_main_calls_sys_do_vblank_process);
    return UNITY_END();
}
