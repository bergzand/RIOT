/*
 * Copyright (C) 2020 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     pkg_openwsn
 * @{
 *
 * @file
 * @brief       Event based implementation of OpenWSN scheduler
 *
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */
#include "event.h"
#include "event/callback.h"
#include "memarray.h"

#include "openwsn_board.h"
#include "scheduler.h"
#include "debugpins.h"
#include "leds.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_ERROR
#endif
#include "log.h"

static scheduler_vars_t _scheduler_vars;
scheduler_dbg_t scheduler_dbg;
/* event queues for every priority */
static event_queue_t _queues[TASKPRIO_MAX];

static event_callback_t *_scheduler_get_free_event(void)
{
    event_callback_t *new = memarray_alloc(&_scheduler_vars.memarray);
    if (!new) {
        /* task list has overflown. This should never happen! */
        LOG_ERROR("[openos/scheduler]: critical, task list overflow\n");
        leds_error_blink();
        board_reset();
    }
    memset(new, 0, sizeof(event_callback_t));
    return new;
}

void scheduler_init(void)
{
    memset(&_scheduler_vars, 0, sizeof(scheduler_vars_t));
    memset(&scheduler_dbg, 0, sizeof(scheduler_dbg_t));
    memarray_init(&_scheduler_vars.memarray, &_scheduler_vars.task_buff,
                  sizeof(event_callback_t), TASK_LIST_DEPTH);

    event_queues_init_detached(_queues, TASKPRIO_MAX);
}

void scheduler_start(unsigned state)
{
    irq_restore(state);

    /* claim all queues */
    event_queues_claim(_queues, TASKPRIO_MAX);

    /* wait for events */
    event_t *event;
    while ((event = event_wait_multi(_queues, TASKPRIO_MAX))) {
        debugpins_task_clr();
        event->handler(event);
        /* remove from task list */
        memarray_free(&_scheduler_vars.memarray, event);
        scheduler_dbg.numTasksCur--;
        debugpins_task_set();
    }
}

void scheduler_push_task(task_cbt cb, task_prio_t prio)
{
    unsigned state = irq_disable();
    /* get a free event from the queue */
    event_callback_t *event = _scheduler_get_free_event();
    if (event == NULL) {
        return;
    }
    event_callback_init(event, (void (*)(void *)) cb, NULL);
    event_post(&_queues[prio], (event_t *)event);

    scheduler_dbg.numTasksCur++;
    if (scheduler_dbg.numTasksCur > scheduler_dbg.numTasksMax) {
        scheduler_dbg.numTasksMax = scheduler_dbg.numTasksCur;
    }
    irq_restore(state);
}
