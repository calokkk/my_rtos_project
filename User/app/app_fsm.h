#ifndef __APP_FSM_H__
#define __APP_FSM_H__

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

/* 状态枚举 */
typedef enum {
    FSM_STATE_IDLE,
    FSM_STATE_RUNNING,
    FSM_STATE_PAUSE,
    FSM_STATE_ERROR,
    FSM_STATE_STOP
} FSM_STATE_E;

/* 事件枚举 */
typedef enum {
    FSM_EVENT_START,
    FSM_EVENT_PAUSE,
    FSM_EVENT_RESUME,
    FSM_EVENT_STOP,
    FSM_EVENT_ERROR,
    FSM_EVENT_RESET,
} FSM_EVENT_E;

/* 状态转换表项 */
typedef struct {
    FSM_STATE_E current;
    FSM_EVENT_E event;
    FSM_STATE_E next;
    void (*callback)(void *arg);
} FSM_STATE_TRANSITION_T;

/* 状态机实例 */
typedef struct {
    FSM_STATE_E current_state;
    const FSM_STATE_TRANSITION_T *trans_tbl;
    uint8_t trans_count;
    QueueHandle_t msg_queue;
    SemaphoreHandle_t mutex;
} STATE_MACHINE_T;

/* 事件消息 */
typedef struct {
    FSM_EVENT_E event;
    void *arg;
    TickType_t ts;
} EVENT_MSG_T;

/* 公开接口 */
uint8_t fsm_start(FSM_STATE_E init_state, uint8_t queue_len);
uint8_t fsm_start_with_simulate(FSM_STATE_E init_state, uint8_t queue_len);
STATE_MACHINE_T* fsm_get_instance(void);

#endif
