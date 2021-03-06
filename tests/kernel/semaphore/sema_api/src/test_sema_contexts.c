/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @addtogroup t_sema_api
 * @{
 * @defgroup t_sema_api_basic test_sema_api_basic
 * @brief TestPurpose: verify zephyr sema apis across different context
 * - API coverage
 *   -# k_sem_init K_SEMA_DEFINE
 *   -# k_sem_take k_sema_give k_sema_reset
 *   -# k_sem_count_get
 * @}
 */

#include <ztest.h>
#include <irq_offload.h>

#define TIMEOUT 100
#define STACK_SIZE 512
#define SEM_INITIAL 0
#define SEM_LIMIT 2
/**TESTPOINT: init via K_SEM_DEFINE*/
K_SEM_DEFINE(ksema, SEM_INITIAL, SEM_LIMIT);
static struct k_sem sema;
static char __noinit __stack tstack[STACK_SIZE];

/*entry of contexts*/
static void tIsr_entry(void *p)
{
	k_sem_give((struct k_sem *)p);
}

static void tThread_entry(void *p1, void *p2, void *p3)
{
	k_sem_give((struct k_sem *)p1);
}

static void tsema_thread_thread(struct k_sem *psem)
{
	/**TESTPOINT: thread-thread sync via sema*/
	k_tid_t tid = k_thread_spawn(tstack, STACK_SIZE,
				     tThread_entry, psem, NULL, NULL,
				     K_PRIO_PREEMPT(0), 0, 0);

	assert_false(k_sem_take(psem, K_FOREVER), NULL);
	/*clean the spawn thread avoid side effect in next TC*/
	k_thread_abort(tid);
}

static void tsema_thread_isr(struct k_sem *psem)
{
	/**TESTPOINT: thread-isr sync via sema*/
	irq_offload(tIsr_entry, psem);
	assert_false(k_sem_take(psem, K_FOREVER), NULL);
}

/*test cases*/
void test_sema_thread2thread(void)
{
	struct k_sem sema;

	/**TESTPOINT: test k_sem_init sema*/
	k_sem_init(&sema, SEM_INITIAL, SEM_LIMIT);
	tsema_thread_thread(&sema);

	/**TESTPOINT: test K_SEM_DEFINE sema*/
	tsema_thread_thread(&ksema);
}

void test_sema_thread2isr(void)
{
	/**TESTPOINT: test k_sem_init sema*/
	k_sem_init(&sema, SEM_INITIAL, SEM_LIMIT);
	tsema_thread_isr(&sema);

	/**TESTPOINT: test K_SEM_DEFINE sema*/
	tsema_thread_isr(&ksema);
}

void test_sema_reset(void)
{
	k_sem_init(&sema, SEM_INITIAL, SEM_LIMIT);
	k_sem_give(&sema);
	k_sem_reset(&sema);
	assert_false(k_sem_count_get(&sema), NULL);
	/**TESTPOINT: sem take return -EBUSY*/
	assert_equal(k_sem_take(&sema, K_NO_WAIT), -EBUSY, NULL);
	/**TESTPOINT: sem take return -EAGAIN*/
	assert_equal(k_sem_take(&sema, TIMEOUT), -EAGAIN, NULL);
	k_sem_give(&sema);
	assert_false(k_sem_take(&sema, K_FOREVER), NULL);
}

void test_sema_count_get(void)
{
	k_sem_init(&sema, SEM_INITIAL, SEM_LIMIT);
	/**TESTPOINT: sem count get upon init*/
	assert_equal(k_sem_count_get(&sema), SEM_INITIAL, NULL);
	k_sem_give(&sema);
	/**TESTPOINT: sem count get after give*/
	assert_equal(k_sem_count_get(&sema), SEM_INITIAL + 1, NULL);
	k_sem_take(&sema, K_FOREVER);
	/**TESTPOINT: sem count get after take*/
	for (int i = 0; i < SEM_LIMIT; i++) {
		assert_equal(k_sem_count_get(&sema), SEM_INITIAL + i, NULL);
		k_sem_give(&sema);
	}
	/**TESTPOINT: sem give above limit*/
	k_sem_give(&sema);
	assert_equal(k_sem_count_get(&sema), SEM_LIMIT, NULL);
}
