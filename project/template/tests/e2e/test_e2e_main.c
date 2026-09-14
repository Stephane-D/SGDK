#include "genesis.h"
#include "unity.h"

static u16 vblankProcessCalls;

static bool testLoopShouldContinue(void)
{
    return vblankProcessCalls <= 100;
}

static bool testSysDoVBlankProcess(void)
{
    vblankProcessCalls++;
    SYS_doVBlankProcess();
    return TRUE;
}

/* Replace hardware operations so the application main can run once on the host. */
#define main applicationMain
#define TRUE testLoopShouldContinue()
#define SYS_doVBlankProcess() testSysDoVBlankProcess()
#include "../../src/main.c"
#undef SYS_doVBlankProcess
#undef TRUE
#undef main

void setUp(void)
{
    vblankProcessCalls = 0;
}

void tearDown(void)
{
}

void test_main_calls_sys_do_vblank_process(void)
{
    applicationMain(FALSE);

    TEST_ASSERT_GREATER_OR_EQUAL_UINT16(100, vblankProcessCalls);
}

int main(bool hardReset)
{
    (void)hardReset;
    UNITY_BEGIN();
    RUN_TEST(test_main_calls_sys_do_vblank_process);
    return UNITY_END();
}
