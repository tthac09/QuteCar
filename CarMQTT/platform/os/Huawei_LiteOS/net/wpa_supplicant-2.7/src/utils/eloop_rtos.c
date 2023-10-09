/*
 * Event loop based on select() loop
 * Copyright (c) 2002-2009, Jouni Malinen <j@w1.fi>
 * Copyright (c) Huawei Technologies Co., Ltd. 2014-2019. All rights reserved.
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */
/****************************************************************************
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations,
 * which might include those applicable to Huawei LiteOS of U.S. and the country in
 * which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in
 * compliance with such applicable export control laws and regulations.
 ****************************************************************************/

#include <assert.h>

#include "common.h"
#include "trace.h"
#include "list.h"
#include "eloop.h"
#include "los_mux.h"
#include "los_hwi.h"
#include "wpa_supplicant/wpa_supplicant.h"
#include "hostapd/hostapd_if.h"
#include "wpa_supplicant/wifi_api.h"
#include "wpa_supplicant/ctrl_iface.h"
#include <hi_cpu.h>
#include <hi_task.h>

#define ELOOP_MAX_EVENTS               16
#define ELOOP_ALREADY_RUNNING          1
#define ELOOP_NOT_RUNNING              0
#define ELOOP_TIMEOUT_REGISTERED       1
#ifndef OSS_TICKS_PER_SEC
#define OSS_TICKS_PER_SEC              100
#endif
#define TICKS_TO_MSEC(ticks) (((ticks) * 1000) / OSS_TICKS_PER_SEC)
#define MSEC_TO_TICKS(ms)    (((ms) * OSS_TICKS_PER_SEC) / 1000)

struct eloop_timeout {
	struct dl_list list;
	struct os_reltime time;
	void *eloop_data;
	void *user_data;
	eloop_timeout_handler handler;
	WPA_TRACE_REF(eloop);
	WPA_TRACE_REF(user);
	WPA_TRACE_INFO
};

struct eloop_message {
	struct dl_list list;
	char *buf;
};

struct eloop_event {
	void *eloop_data;
	void *user_data;
	eloop_event_handler handler;
	unsigned long flag;
	struct dl_list message;
};

struct eloop_data {
	struct dl_list timeout;
	unsigned int mutex_handle;

	int event_count;
	struct eloop_event *events;
	EVENT_CB_S levent;
	unsigned long flags;
};

static struct eloop_data eloop;
static int g_eloop_task_flag[ELOOP_MAX_TASK_TYPE_NUM] = {0};
static int g_eloop_softap_terminate_flag = WPA_FLAG_OFF;
static int g_eloop_wpa_terminate_flag = WPA_FLAG_OFF;
int eloop_is_running(eloop_task_type eloop_task)
{
	if ((eloop_task < 0) || (eloop_task >= ELOOP_MAX_TASK_TYPE_NUM)) {
		return HISI_FAIL;
	}
	LOS_TaskLock();
	if (g_eloop_task_flag[eloop_task] != WPA_FLAG_ON) {
		LOS_TaskUnlock();
		return HISI_FAIL;
	}
	LOS_TaskUnlock();
	return HISI_OK;
}
int eloop_init(eloop_task_type eloop_task)
{
	if ((eloop_task != ELOOP_TASK_WPA) && (eloop_task != ELOOP_TASK_HOSTAPD)) {
		return HISI_FAIL;
	}

	if (eloop.events != NULL) {
		wpa_printf(MSG_INFO, "eloop is already running\n");
		return HISI_OK;
	}
	os_memset(&eloop, 0, sizeof(eloop));
	(void)LOS_EventInit(&eloop.levent);
	dl_list_init(&eloop.timeout);

	unsigned int ret = LOS_MuxCreate(&eloop.mutex_handle);
	if (ret != 0) {
		wpa_error_log0(MSG_ERROR, "eloop create mux fail");
		return HISI_FAIL;
	}

	eloop.events = os_malloc(sizeof(struct eloop_event) * ELOOP_MAX_EVENTS);
	if (eloop.events == NULL) {
		(void)LOS_MuxDelete(eloop.mutex_handle);
		return HISI_FAIL;
	}
	os_memset(eloop.events, 0, sizeof(struct eloop_event) * ELOOP_MAX_EVENTS);
	return HISI_OK;
}

int eloop_register_timeout(unsigned int secs, unsigned int usecs,
			   eloop_timeout_handler handler,
			   void *eloop_data, void *user_data)
{
	struct eloop_timeout *timeout = NULL;
	struct eloop_timeout *tmp = NULL;
	os_time_t now_sec;
	if (handler == NULL)
		return HISI_FAIL;
	timeout = os_zalloc(sizeof(*timeout));
	if (timeout == NULL)
		return HISI_FAIL;
	if (os_get_reltime(&timeout->time) < 0) {
		os_free(timeout);
		return HISI_FAIL;
	}
	now_sec = timeout->time.sec;
	timeout->time.sec += secs;
	if (timeout->time.sec < now_sec) {
		/*
		 * Integer overflow - assume long enough timeout to be assumed
		 * to be infinite, i.e., the timeout would never happen.
		 */
		os_free(timeout);
		return HISI_OK;
	}
	timeout->time.usec += usecs;
	while (timeout->time.usec >= 1000000) {
		timeout->time.sec++;
		timeout->time.usec -= 1000000;
	}
	timeout->eloop_data = eloop_data;
	timeout->user_data = user_data;
	timeout->handler = handler;
	wpa_trace_add_ref(timeout, eloop, eloop_data);
	wpa_trace_add_ref(timeout, user, user_data);
	wpa_trace_record(timeout);

	/* Maintain timeouts in order of increasing time */
	(void)LOS_MuxPend(eloop.mutex_handle, LOS_WAIT_FOREVER);
	dl_list_for_each(tmp, &eloop.timeout, struct eloop_timeout, list) {
		if (os_reltime_before(&timeout->time, &tmp->time)) {
			dl_list_add(tmp->list.prev, &timeout->list);
			(void)LOS_MuxPost(eloop.mutex_handle);
			return HISI_OK;
		}
	}
	dl_list_add_tail(&eloop.timeout, &timeout->list);
	(void)LOS_MuxPost(eloop.mutex_handle);
	return HISI_OK;
}

static void eloop_remove_timeout(struct eloop_timeout *timeout)
{
	(void)LOS_MuxPend(eloop.mutex_handle, LOS_WAIT_FOREVER);
	dl_list_del(&timeout->list);
	(void)LOS_MuxPost(eloop.mutex_handle);

	wpa_trace_remove_ref(timeout, eloop, timeout->eloop_data);
	wpa_trace_remove_ref(timeout, user, timeout->user_data);
	os_free(timeout);
}

int eloop_cancel_timeout(eloop_timeout_handler handler,
			 void *eloop_data, void *user_data)
{
	struct eloop_timeout *timeout = NULL;
	struct eloop_timeout *prev = NULL;
	int removed = 0;

	(void)LOS_MuxPend(eloop.mutex_handle, LOS_WAIT_FOREVER);
	dl_list_for_each_safe(timeout, prev, &eloop.timeout,
				  struct eloop_timeout, list) {
		if (timeout->handler == handler &&
			(timeout->eloop_data == eloop_data ||
			 eloop_data == ELOOP_ALL_CTX) &&
			(timeout->user_data == user_data ||
			 user_data == ELOOP_ALL_CTX)) {
			eloop_remove_timeout(timeout);
			removed++;
		}
	}
	(void)LOS_MuxPost(eloop.mutex_handle);

	return removed;
}

int eloop_cancel_timeout_one(eloop_timeout_handler handler,
				 void *eloop_data, void *user_data,
				 struct os_reltime *remaining)
{
	struct eloop_timeout *timeout = NULL;
	struct eloop_timeout *prev = NULL;
	int removed = 0;
	struct os_reltime now;

	(void)os_get_reltime(&now);
	remaining->sec = remaining->usec = 0;

	(void)LOS_MuxPend(eloop.mutex_handle, LOS_WAIT_FOREVER);
	dl_list_for_each_safe(timeout, prev, &eloop.timeout,
				  struct eloop_timeout, list) {
		if (timeout->handler == handler &&
			(timeout->eloop_data == eloop_data) &&
			(timeout->user_data == user_data)) {
			removed = 1;
			if (os_reltime_before(&now, &timeout->time))
				os_reltime_sub(&timeout->time, &now, remaining);
			eloop_remove_timeout(timeout);
			break;
		}
	}
	(void)LOS_MuxPost(eloop.mutex_handle);
	return removed;
}

int eloop_is_timeout_registered(eloop_timeout_handler handler,
				void *eloop_data, void *user_data)
{
	struct eloop_timeout *tmp = NULL;

	(void)LOS_MuxPend(eloop.mutex_handle, LOS_WAIT_FOREVER);
	dl_list_for_each(tmp, &eloop.timeout, struct eloop_timeout, list) {
		if (tmp->handler == handler &&
			tmp->eloop_data == eloop_data &&
			tmp->user_data == user_data) {
			(void)LOS_MuxPost(eloop.mutex_handle);
			return ELOOP_TIMEOUT_REGISTERED;
		}
	}
	(void)LOS_MuxPost(eloop.mutex_handle);
	return HISI_OK;
}

int eloop_deplete_timeout(unsigned int req_secs, unsigned int req_usecs,
			  eloop_timeout_handler handler, void *eloop_data,
			  void *user_data)
{
	struct os_reltime now, requested, remaining;
	struct eloop_timeout *tmp = NULL;

	(void)LOS_MuxPend(eloop.mutex_handle, LOS_WAIT_FOREVER);
	dl_list_for_each(tmp, &eloop.timeout, struct eloop_timeout, list) {
		if (tmp->handler == handler &&
			tmp->eloop_data == eloop_data &&
			tmp->user_data == user_data) {
			requested.sec = req_secs;
			requested.usec = req_usecs;
			(void)os_get_reltime(&now);
			os_reltime_sub(&tmp->time, &now, &remaining);
			if (os_reltime_before(&requested, &remaining)) {
				(void)eloop_cancel_timeout(handler, eloop_data,
							 user_data);
				(void)eloop_register_timeout(requested.sec,
							   requested.usec,
							   handler, eloop_data,
							   user_data);
				(void)LOS_MuxPost(eloop.mutex_handle);
				return ELOOP_TIMEOUT_REGISTERED;
			}
			(void)LOS_MuxPost(eloop.mutex_handle);
			return HISI_OK;
		}
	}
	(void)LOS_MuxPost(eloop.mutex_handle);
	return HISI_FAIL;
}

int eloop_replenish_timeout(unsigned int req_secs, unsigned int req_usecs,
				eloop_timeout_handler handler, void *eloop_data,
				void *user_data)
{
	struct os_reltime now, requested, remaining;
	struct eloop_timeout *tmp = NULL;

	(void)LOS_MuxPend(eloop.mutex_handle, LOS_WAIT_FOREVER);
	dl_list_for_each(tmp, &eloop.timeout, struct eloop_timeout, list) {
		if (tmp->handler == handler &&
			tmp->eloop_data == eloop_data &&
			tmp->user_data == user_data) {
			requested.sec = req_secs;
			requested.usec = req_usecs;
			(void)os_get_reltime(&now);
			os_reltime_sub(&tmp->time, &now, &remaining);
			if (os_reltime_before(&remaining, &requested)) {
				(void)eloop_cancel_timeout(handler, eloop_data,
							 user_data);
				(void)eloop_register_timeout(requested.sec,
							   requested.usec,
							   handler, eloop_data,
							   user_data);
				(void)LOS_MuxPost(eloop.mutex_handle);
				return ELOOP_TIMEOUT_REGISTERED;
			}
			(void)LOS_MuxPost(eloop.mutex_handle);
			return HISI_OK;
		}
	}
	(void)LOS_MuxPost(eloop.mutex_handle);
	return HISI_FAIL;
}
static int eloop_terminate_wpa_process(void)
{
	int count = 0;
	if (g_eloop_wpa_terminate_flag == WPA_FLAG_ON) {
		g_eloop_wpa_terminate_flag = WPA_FLAG_OFF;
		count++;
		hi_wpa_supplicant_exit();
		wpa_error_log0(MSG_ERROR, "hi_wpa_supplicant_exit, send WPA_EVENT_STA_STOP_OK\n");
		(void)LOS_EventWrite(&g_wpa_event, WPA_EVENT_STA_STOP_OK);
	}
	if (g_eloop_softap_terminate_flag == WPA_FLAG_ON) {
		g_eloop_softap_terminate_flag = WPA_FLAG_OFF;
		count++;
		hi_hostapd_exit();
		// send softap stop event
		wpa_error_log0(MSG_ERROR, "hi_hostapd_exit, send WPA_EVENT_AP_STOP_OK\n");
		(void)LOS_EventWrite(&g_wpa_event, WPA_EVENT_AP_STOP_OK);
	}
	return count ? HISI_OK : HISI_FAIL;
}

static inline void eloop_timeout_event_process(struct eloop_timeout *timeout)
{
	void *eloop_data = timeout->eloop_data;
	void *user_data = timeout->user_data;
	eloop_timeout_handler handler = timeout->handler;
	eloop_remove_timeout(timeout);
	handler(eloop_data, user_data);
}

static inline void eloop_run_process_timeout_event(void)
{
	struct eloop_timeout *timeout = NULL;
	struct os_reltime now;
	/* check if some registered timeouts have occurred */
	timeout = dl_list_first(&eloop.timeout, struct eloop_timeout, list);
	if (timeout) {
		(void)os_get_reltime(&now);
		if (!os_reltime_before(&now, &timeout->time)) {
			eloop_timeout_event_process(timeout);
		}
	}
}

static inline void eloop_run_process_normal_event(unsigned int *ret_flags)
{
	if (ret_flags == NULL)
		return;
	for (int i = 0; i < ELOOP_MAX_EVENTS; i++) {
		if (eloop.events[i].flag & *ret_flags) {
			if (eloop.events[i].handler)
				eloop.events[i].handler(eloop.events[i].eloop_data, eloop.events[i].user_data);
			*ret_flags &= ~eloop.events[i].flag;
			if (eloop.events[i].handler == (eloop_event_handler)eloop_terminate)
				break;
		}
	}
}

static inline int eloop_run_check(eloop_task_type eloop_task)
{
	unsigned int ret = 0;
	for (int i = 0; i < ELOOP_MAX_TASK_TYPE_NUM; i++) {
		ret |= (unsigned int)(g_eloop_task_flag[i] == WPA_FLAG_ON);
		if (ret) {
			g_eloop_task_flag[eloop_task] = WPA_FLAG_ON;
			wpa_error_log1(MSG_ERROR, "ELOOP_ALREADY_RUNNING: eloop_task= %d", eloop_task);
			(void)LOS_EventWrite(&g_wpa_event, WPA_EVENT_WPA_START_OK);
			return ELOOP_ALREADY_RUNNING;
		}
	}
	return ELOOP_NOT_RUNNING;
}

int eloop_run(eloop_task_type eloop_task)
{
	unsigned long time;
	struct os_reltime tv, now;
	int start_flag = WPA_FLAG_ON;

	if (eloop_task >= ELOOP_MAX_TASK_TYPE_NUM) {
		return HISI_FAIL;
	}

	if (eloop_run_check(eloop_task) == ELOOP_NOT_RUNNING) {
		g_eloop_task_flag[eloop_task] = WPA_FLAG_ON;
	} else {
		return ELOOP_ALREADY_RUNNING;
	}

	while ((g_eloop_task_flag[ELOOP_TASK_WPA] == WPA_FLAG_ON) || (g_eloop_task_flag[ELOOP_TASK_HOSTAPD] == WPA_FLAG_ON)) {
		unsigned int ret_flags;
		struct eloop_timeout *timeout = NULL;
		if (start_flag) {
			wpa_error_log1(MSG_ERROR, "eloop_run first: eloop_task= %d", eloop_task);
			start_flag = WPA_FLAG_OFF;
			(void)LOS_EventWrite(&g_wpa_event, WPA_EVENT_WPA_START_OK);
		}
		hi_cpup_load_check_proc(hi_task_get_current_id(), LOAD_SLEEP_TIME_DEFAULT);

		timeout = dl_list_first(&eloop.timeout, struct eloop_timeout, list);
		if (timeout != NULL) {
			(void)os_get_reltime(&now);
			if (os_reltime_before(&now, &timeout->time))
				os_reltime_sub(&timeout->time, &now, &tv);
			else
				tv.sec = tv.usec = 0;
			time = tv.sec * 1000 + tv.usec / 1000;
			if (time) {
				ret_flags = LOS_EventRead(&eloop.levent, eloop.flags, LOS_WAITMODE_OR, MSEC_TO_TICKS(time));
			} else {
				ret_flags = LOS_EventPoll(&eloop.levent.uwEventID, eloop.flags, LOS_WAITMODE_OR);
			}
		} else {
			ret_flags = LOS_EventRead(&eloop.levent, eloop.flags, LOS_WAITMODE_OR, LOS_WAIT_FOREVER);
		}

		(void)LOS_EventClear(&eloop.levent, ~ret_flags);

		eloop_run_process_timeout_event();
		eloop_run_process_normal_event(&ret_flags);

		if (eloop_terminate_wpa_process() == HISI_OK) {
			wpa_error_log0(MSG_ERROR, "eloop_terminate_wpa_process is called in eloop_run while\n");
			ret_flags = 0;
			continue;
		}
	}
	eloop_terminate_wpa_process();
	return HISI_OK;
}

void eloop_terminate(eloop_task_type eloop_task)
{
	if ((eloop_task != ELOOP_TASK_WPA) && (eloop_task != ELOOP_TASK_HOSTAPD)) {
		return;
	}
	wpa_error_log1(MSG_ERROR, "eloop_terminate : %d", eloop_task);
	if (g_eloop_task_flag[eloop_task] != WPA_FLAG_ON) {
		return;
	}
	g_eloop_task_flag[eloop_task] = WPA_FLAG_OFF;

	if (eloop_task == ELOOP_TASK_WPA) {
		g_eloop_wpa_terminate_flag = WPA_FLAG_ON;
	} else {
		g_eloop_softap_terminate_flag = WPA_FLAG_ON;
	}
}

void eloop_destroy(eloop_task_type eloop_task)
{
	struct eloop_timeout *timeout = NULL;
	struct eloop_timeout *prev = NULL;
	struct os_reltime now;
	int i;
	int ret = 0;
	if ((eloop_task != ELOOP_TASK_WPA) && (eloop_task != ELOOP_TASK_HOSTAPD)) {
		return;
	}
	for (i = 0; i < ELOOP_MAX_TASK_TYPE_NUM; i++) {
		ret |= (g_eloop_task_flag[i] == WPA_FLAG_ON);
		if (ret) {
			return;
		}
	}

	(void)os_get_reltime(&now);
	(void)LOS_MuxPend(eloop.mutex_handle, LOS_WAIT_FOREVER);
	dl_list_for_each_safe(timeout, prev, &eloop.timeout,
				  struct eloop_timeout, list) {
		int sec, usec;
		sec = timeout->time.sec - now.sec;
		usec = timeout->time.usec - now.usec;
		if (timeout->time.usec < now.usec) {
			sec--;
			usec += 1000000;
		}
		eloop_remove_timeout(timeout);
	}
	(void)LOS_MuxPost(eloop.mutex_handle);
	os_free(eloop.events);
	eloop.events = NULL;
	LOS_MuxDelete(eloop.mutex_handle);
}

int eloop_terminated(eloop_task_type eloop_task)
{
	if ((eloop_task != ELOOP_TASK_WPA) && (eloop_task != ELOOP_TASK_HOSTAPD)) {
		return HISI_FAIL;
	}
	return g_eloop_task_flag[eloop_task];
}

int eloop_register_event(void *event, size_t event_size,
			 eloop_event_handler handler, void *eloop_data, void *user_data)
{
	unsigned int i;
	struct eloop_event *tmp = NULL;

	(void)event_size;
	if (eloop.event_count >= ELOOP_MAX_EVENTS) {
		wpa_error_log1(MSG_ERROR, "eloop_register_event enter, eloop.event_count = %d.", eloop.event_count);
		return HISI_FAIL;
	}
	for (i = 0; i < ELOOP_MAX_EVENTS; i++) {
		if (eloop.events[i].flag == 0) {
			tmp = &eloop.events[i];
			break;
		}
	}
	if ((i == ELOOP_MAX_EVENTS) || (tmp == NULL)) {
		wpa_error_log0(MSG_ERROR, "eloop_register_event enter, no events can be used.");
		return HISI_FAIL;
	}

	tmp->eloop_data = eloop_data;
	tmp->user_data = user_data;
	tmp->handler = handler;
	tmp->flag = 1 << i;
	dl_list_init(&tmp->message);

	eloop.flags |= tmp->flag;
	eloop.event_count++;

	if (event)
		*(struct eloop_event **)event = tmp;

	return HISI_OK;
}

void eloop_unregister_event(void *event, size_t size)
{
	int i;
	struct eloop_event *tmp = (struct eloop_event *)event;
	struct eloop_message *message = NULL;

	(void)size;
	if ((tmp == NULL) || (eloop.event_count == 0)) {
		wpa_error_log1(MSG_ERROR, "eloop_unregister_event, eloop.event_count = %d.", eloop.event_count);
		return;
	}
	(void)LOS_MuxPend(eloop.mutex_handle, LOS_WAIT_FOREVER);
	for (i = 0; i < ELOOP_MAX_EVENTS; i++) {
		if (&eloop.events[i] == tmp)
			break;
	}
	if (i == ELOOP_MAX_EVENTS) {
		(void)LOS_MuxPost(eloop.mutex_handle);
		return;
	}
	eloop.flags &= ~tmp->flag;
	(void)LOS_MuxPost(eloop.mutex_handle);
	tmp->flag = 0;
	tmp->handler = NULL;

	while (1) {
		message = dl_list_first(&tmp->message, struct eloop_message, list);
		if (message == NULL)
			break;

		dl_list_del(&message->list);
		os_free(message->buf);
		os_free(message);
	}

	eloop.event_count--;
}

void eloop_unregister_cli_event(void *event, size_t size)
{
	int i;
	struct eloop_event *tmp = (struct eloop_event *)event;
	struct eloop_message *message = NULL;
	(void)size;
	if ((tmp == NULL) || (eloop.event_count == 0)) {
		wpa_error_log1(MSG_ERROR, "eloop_unregister_cli_event, eloop.event_count = %d.", eloop.event_count);
		return;
	}

	(void)LOS_MuxPend(eloop.mutex_handle, LOS_WAIT_FOREVER);
	for (i = 0; i < ELOOP_MAX_EVENTS; i++) {
		if (&eloop.events[i] == tmp)
			break;
	}
	if (i == ELOOP_MAX_EVENTS) {
		(void)LOS_MuxPost(eloop.mutex_handle);
		return;
	}
	eloop.flags &= ~tmp->flag;
	(void)LOS_MuxPost(eloop.mutex_handle);
	tmp->flag = 0;
	tmp->handler = NULL;

	while (1) {
		message = dl_list_first(&tmp->message, struct eloop_message, list);
		if (message == NULL)
			break;

		dl_list_del(&message->list);
		if (message->buf != NULL)
			os_free(((struct ctrl_iface_event_buf *)(message->buf))->buf);
		os_free(message->buf);
		os_free(message);
	}
	eloop.event_count--;
}

int eloop_post_event(void *event, void *buf, int set_event)
{
	unsigned int int_save;
	int i;
	struct eloop_event *tmp = (struct eloop_event *)event;
	struct eloop_message *message = NULL;

	(void)set_event;
	if (tmp == NULL) {
		return HISI_FAIL;
	}
	for (i = 0; i < ELOOP_MAX_EVENTS; i++) {
		if (&eloop.events[i] == tmp)
			break;
	}
	if (i == ELOOP_MAX_EVENTS) {
		return HISI_FAIL;
	}
	message = os_malloc(sizeof(struct eloop_message));
	if (message == NULL) {
		return HISI_FAIL;
	}
	message->buf = buf;

	int_save = LOS_IntLock();
	dl_list_add_tail(&tmp->message, &message->list);
	LOS_IntRestore(int_save);
	(void)LOS_EventWrite(&eloop.levent, tmp->flag);
	return HISI_OK;
}

void *eloop_read_event(void *event, int timeout)
{
	unsigned int int_save;
	void *ret = NULL;
	int i;
	struct eloop_event *tmp = (struct eloop_event *)event;
	struct eloop_message *message = NULL;

	(void)timeout;
	if (tmp == NULL) {
		return NULL;
	}
	for (i = 0; i < ELOOP_MAX_EVENTS; i++) {
		if (&eloop.events[i] == tmp)
			break;
	}
	if (i == ELOOP_MAX_EVENTS) {
		return NULL;
	}
	message = dl_list_first(&tmp->message, struct eloop_message, list);
	if (message != NULL) {
		ret = message->buf; // copy data before free.

		int_save = LOS_IntLock();
		dl_list_del(&message->list);
		LOS_IntRestore(int_save);
		os_free(message);
	}
	return ret;
}
