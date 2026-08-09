#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

// 1. 内核配置
#define configUSE_PREEMPTION            1       // 启用抢占式调度
#define configUSE_IDLE_HOOK             0       // 禁用空闲钩子函数
#define configUSE_TICK_HOOK             0       // 禁用tick钩子函数
#define configCPU_CLOCK_HZ              (72000000) // STM32系统时钟（F103为72MHz）
#define configTICK_RATE_HZ              (1000)  // Tick频率（1ms/tick）
#define configMAX_PRIORITIES            (16)    // 最大任务优先级（0最低，15最高）
#define configMINIMAL_STACK_SIZE        (128)   // 空闲任务栈大小
#define configTOTAL_HEAP_SIZE           ((size_t)(10*1024)) // 堆大小（10KB，按需调整）
#define configMAX_TASK_NAME_LEN         (16)    // 任务名最大长度
#define configUSE_TRACE_FACILITY        0       // 禁用追踪
#define configUSE_16_BIT_TICKS          0       // 32位tick计数
#define configIDLE_SHOULD_YIELD         1       // 空闲任务让步

// 2. 中断优先级配置（关键！和NVIC分组4匹配）
#define configKERNEL_INTERRUPT_PRIORITY         0xe0 // 内核中断优先级（抢占优先级15，最低）
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    0x50 // 系统调用允许的最高中断优先级（抢占优先级5）

// 3. 功能模块开关
#define configUSE_TIMERS                1       // 启用软件定时器
#define configTIMER_TASK_PRIORITY       (configMAX_PRIORITIES - 1) // 定时器任务优先级
#define configTIMER_QUEUE_LENGTH        10      // 定时器命令队列长度
#define configTIMER_TASK_STACK_DEPTH    (configMINIMAL_STACK_SIZE) // 定时器任务栈大小
#define configQUEUE_REGISTRY_SIZE       0       // 禁用队列注册
#define configUSE_MUTEXES               1       // 启用互斥锁
#define configUSE_RECURSIVE_MUTEXES     1       // 启用递归互斥锁
#define configUSE_COUNTING_SEMAPHORES   1       // 启用计数信号量

// 4. 钩子函数配置
#define configCHECK_FOR_STACK_OVERFLOW  0       // 禁用栈溢出检测

// 5. 系统调用宏（必须定义）
#define INCLUDE_vTaskPrioritySet        1
#define INCLUDE_uxTaskPriorityGet       1
#define INCLUDE_vTaskDelete             1
#define INCLUDE_vTaskCleanUpResources   1
#define INCLUDE_vTaskSuspend            1
#define INCLUDE_vTaskDelayUntil         1
#define INCLUDE_vTaskDelay              1

// 6.中断服务函数映射（让FreeRTOS接管SysTick/PendSV/SVC中断）
#define xPortSysTickHandler SysTick_Handler
#define xPortPendSVHandler PendSV_Handler
#define vPortSVCHandler SVC_Handler

#endif /* FREERTOS_CONFIG_H */
