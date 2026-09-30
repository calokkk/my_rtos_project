#include "stm32f10x.h"                  // Device header
#include "OLED.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "string.h"

#define OFFLINE_LOG_BUF_LIST_NUM        2
#define OFFLINE_LOG_BUF_MAX_SIZE        0x800 //2kb
#define OFFLINE_LOG_SN_MAX_NUM          0xffffffff

typedef enum{
    OFFLINE_LOG_BUF_STATE_FREE,
    OFFLINE_LOG_BUF_STATE_WRITING,
}OFFLINE_LOG_BUF_STATE_E;

typedef struct
{
    uint32_t sn;
    OFFLINE_LOG_BUF_STATE_E state;
    uint16_t offset;
    uint8_t buf[OFFLINE_LOG_BUF_MAX_SIZE];
}OFFLINE_lOG_BUF_T;


OFFLINE_lOG_BUF_T offline_log_buf_list[OFFLINE_LOG_BUF_LIST_NUM];
uint8_t current_log_buf_list_index;
uint32_t last_write_sn;
OFFLINE_lOG_BUF_T *data_buf; 

void offline_log_init()
{
    current_log_buf_list_index = 0;
    last_write_sn = 0;

    for(uint8_t i=0; i<OFFLINE_LOG_BUF_LIST_NUM; i++)
    {
        offline_log_buf_list[i].sn = OFFLINE_LOG_SN_MAX_NUM;
        offline_log_buf_list[i].state = OFFLINE_LOG_BUF_STATE_FREE;
        offline_log_buf_list[i].offset = 0;
        memset(offline_log_buf_list[i].buf, 0, OFFLINE_LOG_BUF_MAX_SIZE);
    }
}

void offline_log_trace_data_handler(uint8_t *buf, uint16_t size)
{
    uint16_t remain_size;
    uint16_t write_size;
    uint16_t writen_size;

    data_buf = &offline_log_buf_list[current_log_buf_list_index];
    remain_size = size;

    // check the size oversize ?
    if((data_buf->offset + size) > OFFLINE_LOG_BUF_MAX_SIZE)
    {
        write_size = OFFLINE_LOG_BUF_MAX_SIZE - data_buf->offset;
    }
    else
    {
        write_size = remain_size;
    }

    if(write_size > 0)
    {
        writen_size = 0;

        do
        {
            if(data_buf->state == OFFLINE_LOG_BUF_STATE_FREE)
            {
                data_buf->state = OFFLINE_LOG_BUF_STATE_WRITING;
                data_buf->offset = 0;
                // memset(data_buf->buf, 0, OFFLINE_LOG_BUF_MAX_SIZE);
            }
            memcpy(data_buf->buf + data_buf->offset, buf + writen_size, write_size);
            writen_size += write_size;
            data_buf->offset += write_size;

            if(data_buf->offset == OFFLINE_LOG_BUF_MAX_SIZE)
            {
                last_write_sn++;
                data_buf->sn = last_write_sn % OFFLINE_LOG_SN_MAX_NUM;

                data_buf->state = OFFLINE_LOG_BUF_STATE_FREE;
                data_buf->offset = 0;

                // buf full. write to flash
                offline_log_write_to_flash(data_buf->buf, OFFLINE_LOG_BUF_MAX_SIZE);

                current_log_buf_list_index = (current_log_buf_list_index + 1) % OFFLINE_LOG_BUF_LIST_NUM;
                data_buf = &offline_log_buf_list[current_log_buf_list_index];
            }

            remain_size -= write_size;

            if(remain_size < (OFFLINE_LOG_BUF_MAX_SIZE - data_buf->offset))
            {
                write_size = remain_size;
            }
            else
            {
                write_size = OFFLINE_LOG_BUF_MAX_SIZE - data_buf->offset;
            }
            
        }
        while(remain_size > 0);
    }
}

void offline_log_write_to_flash(uint8_t *buf, uint16_t size)
{
    uint16_t sn_min_buf_index;
    uint8_t write_buf[OFFLINE_LOG_BUF_MAX_SIZE];

    sn_min_buf_index = offline_log_buf_list[0].sn < offline_log_buf_list[1].sn ? 0 : 1;

    if((sn_min_buf_index == 0) && (offline_log_buf_list[0].state == OFFLINE_LOG_BUF_STATE_FREE))
    {
        memcpy(write_buf, offline_log_buf_list[0].buf, OFFLINE_LOG_BUF_MAX_SIZE);
        //todo => write tof flash
    }
    else if((sn_min_buf_index == 1) && (offline_log_buf_list[1].state == OFFLINE_LOG_BUF_STATE_FREE))
    {
        memcpy(write_buf, offline_log_buf_list[1].buf, OFFLINE_LOG_BUF_MAX_SIZE);
        //todo => write tof flash
    }


}