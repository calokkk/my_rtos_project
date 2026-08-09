#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stdio.h"
#include "app_fsm.h"
#include "Uart.h"
static uint8_t fsm_init(STATE_MACHINE_T *fsm, FSM_STATE_E init_state, uint8_t queue_len);
void fsm_task(void *pvParameters);
void fsm_simulate_task(void *pvParameters);

static const FSM_STATE_TRANSITION_T tbl[] = {
    {FSM_STATE_IDLE, FSM_EVENT_START, FSM_STATE_RUNNING},

    {FSM_STATE_RUNNING, FSM_EVENT_PAUSE, FSM_STATE_PAUSE},
    {FSM_STATE_RUNNING, FSM_EVENT_ERROR, FSM_STATE_ERROR},
    {FSM_STATE_RUNNING, FSM_EVENT_STOP, FSM_STATE_STOP},

    {FSM_STATE_PAUSE, FSM_EVENT_RESUME, FSM_STATE_RUNNING},
    {FSM_STATE_PAUSE, FSM_EVENT_ERROR, FSM_STATE_ERROR},
    {FSM_STATE_PAUSE, FSM_EVENT_STOP, FSM_STATE_STOP},

    {FSM_STATE_ERROR, FSM_EVENT_RESET, FSM_STATE_IDLE},
};

/* 全局状态机实例 */
static STATE_MACHINE_T g_fsm;

/* 便捷初始化：直接初始化全局实例 */
uint8_t fsm_instance_init(FSM_STATE_E init_state, uint8_t queue_len)
{
    return fsm_init(&g_fsm, init_state, queue_len);
}

/* 获取全局实例指针 */
STATE_MACHINE_T* fsm_get_instance(void)
{
    return &g_fsm;
}

const char* state_to_str(FSM_STATE_E state)
{
    switch (state)
    {
    case FSM_STATE_IDLE:
        return "IDLE";
    
    case FSM_STATE_RUNNING:
        return "RUNNING";

    case FSM_STATE_PAUSE:
        return "PAUSE";

    case FSM_STATE_ERROR:
        return "ERROR";

    case FSM_STATE_STOP:
        return "STOP";

    default:
        return "NONE";
    }
}

static uint8_t fsm_init(STATE_MACHINE_T *fsm, FSM_STATE_E init_state, uint8_t queue_len)
{
    fsm->current_state = init_state;
    fsm->trans_tbl = tbl;
    fsm->trans_count = sizeof(tbl)/sizeof(tbl[0]);

    fsm->msg_queue = xQueueCreate(queue_len, sizeof(EVENT_MSG_T *));
    if(fsm->msg_queue == NULL)
    {
        uart_printf("init fsm queue err");
        return 0;
    }

    fsm->mutex = xSemaphoreCreateMutex();
    if(fsm->mutex == NULL)
    {
        uart_printf("init fsm mutex err");
        return 0;
    }

    return 1;
}

void fsm_msg_send(STATE_MACHINE_T *fsm, FSM_EVENT_E event, void* arg, BaseType_t *xHigherPriorityTaskWoken)
{
    EVENT_MSG_T *msg = pvPortMalloc(sizeof(EVENT_MSG_T));
    if(msg == NULL) return;

    msg->event = event;
    msg->arg   = arg;
    msg->ts    = xTaskGetTickCount();

    if(xHigherPriorityTaskWoken != NULL)
    {
        xQueueSendFromISR(fsm->msg_queue, &msg, xHigherPriorityTaskWoken);
    }
    else
    {
        xQueueSend(fsm->msg_queue, &msg, portMAX_DELAY);
    }
}

/* ==================== 任务上下文事件发送接口 ==================== */
void fsm_send_start(STATE_MACHINE_T *fsm, void *arg)
{
    fsm_msg_send(fsm, FSM_EVENT_START, arg, NULL);
}

void fsm_send_pause(STATE_MACHINE_T *fsm, void *arg)
{
    fsm_msg_send(fsm, FSM_EVENT_PAUSE, arg, NULL);
}

void fsm_send_resume(STATE_MACHINE_T *fsm, void *arg)
{
    fsm_msg_send(fsm, FSM_EVENT_RESUME, arg, NULL);
}

void fsm_send_stop(STATE_MACHINE_T *fsm, void *arg)
{
    fsm_msg_send(fsm, FSM_EVENT_STOP, arg, NULL);
}

void fsm_send_error(STATE_MACHINE_T *fsm, void *arg)
{
    fsm_msg_send(fsm, FSM_EVENT_ERROR, arg, NULL);
}

void fsm_send_reset(STATE_MACHINE_T *fsm, void *arg)
{
    fsm_msg_send(fsm, FSM_EVENT_RESET, arg, NULL);
}

/* ==================== 工具函数 ==================== */
FSM_STATE_E fsm_get_state(STATE_MACHINE_T *fsm)
{
    return fsm->current_state;
}

void fsm_print_state(STATE_MACHINE_T *fsm)
{
    uart_printf("fsm current state: %s\r\n", state_to_str(fsm->current_state));
}

const char* event_to_str(FSM_EVENT_E event)
{
    switch (event)
    {
    case FSM_EVENT_START:  return "START";
    case FSM_EVENT_PAUSE:  return "PAUSE";
    case FSM_EVENT_RESUME: return "RESUME";
    case FSM_EVENT_STOP:   return "STOP";
    case FSM_EVENT_ERROR:  return "ERROR";
    case FSM_EVENT_RESET:  return "RESET";
    default:               return "UNKNOWN";
    }
}

/* ==================== 一键启动：初始化 + 创建任务 ==================== */
#define FSM_TASK_STACK_SIZE    256
#define FSM_TASK_PRIORITY      2
#define FSM_SIM_TASK_STACK_SIZE  256
#define FSM_SIM_TASK_PRIORITY    1

TaskHandle_t g_fsm_task_handle    = NULL;
TaskHandle_t g_fsm_sim_task_handle = NULL;

uint8_t fsm_start(FSM_STATE_E init_state, uint8_t queue_len)
{
    /* 1. 初始化状态机 */
    if(!fsm_instance_init(init_state, queue_len))
    {
        uart_printf("fsm_start: init failed\r\n");
        return 0;
    }

    /* 2. 创建消息处理任务 */
    if(xTaskCreate(fsm_task,
                   "fsm_task",
                   FSM_TASK_STACK_SIZE,
                   fsm_get_instance(),
                   FSM_TASK_PRIORITY,
                   &g_fsm_task_handle) != pdPASS)
    {
        uart_printf("fsm_start: create fsm_task failed\r\n");
        return 0;
    }

    uart_printf("fsm_start: ok, state=%s\r\n", state_to_str(init_state));
    return 1;
}

uint8_t fsm_start_with_simulate(FSM_STATE_E init_state, uint8_t queue_len)
{
    if(!fsm_start(init_state, queue_len))
    {
        return 0;
    }

    /* 创建模拟演示任务 */
    if(xTaskCreate(fsm_simulate_task,
                   "fsm_sim",
                   FSM_SIM_TASK_STACK_SIZE,
                   fsm_get_instance(),
                   FSM_SIM_TASK_PRIORITY,
                   &g_fsm_sim_task_handle) != pdPASS)
    {
        uart_printf("fsm_start_with_simulate: create sim_task failed\r\n");
        return 0;
    }

    return 1;
}

/* ==================== 状态机核心处理 ==================== */
uint8_t fsm_process(STATE_MACHINE_T *fsm, EVENT_MSG_T *msg)
{
    if(xSemaphoreTake(fsm->mutex, portMAX_DELAY) == pdTRUE)
    {
        for(uint8_t i=0; i<fsm->trans_count; i++)
        {
            if((fsm->current_state == fsm->trans_tbl[i].current) && (fsm->trans_tbl[i].event == msg->event))
            {
                if(fsm->trans_tbl[i].callback != NULL)
                {
                    fsm->trans_tbl[i].callback(msg->arg);
                }

                uart_printf("fsm state [ %s ] to [ %s ], event: %d", state_to_str(fsm->current_state), state_to_str(fsm->trans_tbl[i].next), msg->event);

                fsm->current_state = fsm->trans_tbl[i].next;
                xSemaphoreGive(fsm->mutex);
                return 1;
            }
        }
        uart_printf("fsm event:%d err", msg->event);
        xSemaphoreGive(fsm->mutex);
        return 0;
    }

    return 0;
}


void fsm_task(void *pvParameters)
{
    STATE_MACHINE_T *fsm = (STATE_MACHINE_T *)pvParameters;
    EVENT_MSG_T *msg;

    while (1)
    {
        if(xQueueReceive(fsm->msg_queue, &msg, portMAX_DELAY) == pdTRUE)
        {
            fsm_process(fsm, msg);
            vPortFree(msg);
        }
    }
}


/* ==================== 模拟用户操作演示任务 ==================== */
void fsm_simulate_task(void *pvParameters)
{
    STATE_MACHINE_T *fsm = (STATE_MACHINE_T *)pvParameters;

    /* 等 fsm_task 就绪 */
    vTaskDelay(pdMS_TO_TICKS(500));

    uart_printf("=== FSM Simulate Start ===\r\n");

    /* 1. IDLE → START → RUNNING */
    uart_printf("> send START\r\n");
    fsm_send_start(fsm, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));

    /* 2. RUNNING → PAUSE → PAUSE */
    uart_printf("> send PAUSE\r\n");
    fsm_send_pause(fsm, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));

    /* 3. PAUSE → RESUME → RUNNING */
    uart_printf("> send RESUME\r\n");
    fsm_send_resume(fsm, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));

    /* 4. RUNNING → ERROR → ERROR */
    uart_printf("> send ERROR\r\n");
    fsm_send_error(fsm, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));

    /* 5. ERROR → RESET → IDLE */
    uart_printf("> send RESET\r\n");
    fsm_send_reset(fsm, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));

    /* 6. IDLE → START → RUNNING → STOP → STOP */
    uart_printf("> send START\r\n");
    fsm_send_start(fsm, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));

    uart_printf("> send STOP\r\n");
    fsm_send_stop(fsm, NULL);
    vTaskDelay(pdMS_TO_TICKS(100));

    uart_printf("=== FSM Simulate End ===\r\n");

    vTaskDelete(NULL);
}
