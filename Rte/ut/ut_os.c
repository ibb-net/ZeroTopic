
/***************************************************************************
 *
 * Copyright (c) 2021 ZelosTech.com, Inc. All Rights Reserved
 *
 **************************************************************************/

#include "os_timestamp.h"
#include "os_heap.h"
#include "os_thread.h"
#include "os_mutex.h"
#include "os_semaphore.h"

#include "ut_task.h"

static UTCase_t ut_os_sleep(void)
{
    UT_TEST_ENTRY();

    TEST_TIME(os_thread_sleep_ms(1),   0,     5000);
    TEST_TIME(os_thread_sleep_ms(100), 95000, 105000);

    UT_TEST_RETURN();
}

static ssize_t task_os_thread(struct UTTaskDo_s* pDo)
{
    return 0;
}

static UTCase_t ut_os_thread(void)
{
    UT_TEST_ENTRY();

    struct UTTaskDo_s TaskDo = {
        .argc = 0,
        .argv[0] = NULL,
        .ut_case_do = task_os_thread,
        .ret = -1,
    };

    TEST_TASK_PUSH(TaskDo);
    TEST_SLEEP(10);
    TEST_EQ(TaskDo.ret, 0);

    UT_TEST_RETURN();
}

static ssize_t task_os_mutex(struct UTTaskDo_s* pDo)
{
    OsMutex_t* pMutex = NULL;

    if (pDo->argc != 1) {
        return -1;
    }
    pMutex = *((OsMutex_t**)(pDo->argv[0]));

    os_mutex_take(pMutex);
    ut_sleep_ms(100);
    os_mutex_give(pMutex);
    return 0;
}

static UTCase_t ut_os_mutex(void)
{
    UT_TEST_ENTRY();

    OsMutex_t* pMutex = NULL;
    struct UTTaskDo_s TaskDo = {
        .argc = 1,
        .argv[0] = &pMutex,
        .ut_case_do = task_os_mutex,
        .ret = 0,
    };

    TEST_NN(pMutex = os_mutex_create(NULL));
    TEST_NE(pMutex, NULL);
    TEST_FAIL_BREAK();

    TEST_TASK_PUSH(TaskDo);
    TEST_SLEEP(10);
    TEST_TIME(os_mutex_take(pMutex), 80000 - 10000, 120000 - 10000);
    TEST_GE(os_mutex_give(pMutex), 0);
    TEST_GE(os_mutex_destroy(pMutex), 0);

    UT_TEST_RETURN();
}

static ssize_t task_os_semaphore(struct UTTaskDo_s* pDo)
{
    OsSemaphore_t* pSem = NULL;

    if (pDo->argc != 1) {
        return -1;
    }

    pSem = *((OsSemaphore_t**)(pDo->argv[0]));

    ut_sleep_ms(100);
    os_semaphore_give(pSem);
    return 0;
}

static UTCase_t ut_os_semaphore(void)
{
    UT_TEST_ENTRY();

    OsSemaphore_t* pSem = NULL;

    struct UTTaskDo_s TaskDo = {
        .argc = 1,
        .argv[0] = &pSem,
        .ut_case_do = task_os_semaphore,
        .ret = 0,
    };

    TEST_NN(pSem = os_semaphore_create(0, NULL));
    TEST_NE(pSem, NULL);
    TEST_FAIL_BREAK();

    TEST_TASK_PUSH(TaskDo);
    TEST_SLEEP(10);
    TEST_TIME(os_semaphore_take(pSem, 200), 80000 - 10000, 120000 - 10000);
    TEST_TIME(os_semaphore_take(pSem, 100), 80000, 120000);
    TEST_GE(os_semaphore_destroy(pSem), 0);

    UT_TEST_RETURN();
}

UTCaseSet_t ut_os[] = {
    UT_TEST(ut_os_sleep),
    UT_TEST(ut_os_thread),
    UT_TEST(ut_os_mutex),
    UT_TEST(ut_os_semaphore),
    UT_TEST(NULL),
};
