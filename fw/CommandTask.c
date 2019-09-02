/*
 * Written by Sultan Qasim Khan
 * Copyright (c) 2018-2019, NCC Group plc
 * Released as open source under GPLv3
 */

/***** Includes *****/
#include <stdlib.h>
#include <string.h>
#include <xdc/std.h>
#include <xdc/runtime/System.h>

#include <CommandTask.h>
#include <RadioTask.h>
#include <PacketTask.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Event.h>

/* Board Header files */
#include "Board.h"
#include "messenger.h"

/***** Defines *****/
#define COMMAND_TASK_STACK_SIZE 1024
#define COMMAND_TASK_PRIORITY   3

/***** Type declarations *****/


/***** Variable declarations *****/
static Task_Params commandTaskParams;
Task_Struct commandTask; /* not static so you can see in ROV */
static uint8_t commandTaskStack[COMMAND_TASK_STACK_SIZE];
static uint8_t msgBuf[MESSAGE_MAX];

/***** Prototypes *****/
static void commandTaskFunction(UArg arg0, UArg arg1);

/***** Function definitions *****/
void CommandTask_init(void) {
    // I assume PacketTask will initialize the messenger

    /* Create the command handler task */
    Task_Params_init(&commandTaskParams);
    commandTaskParams.stackSize = COMMAND_TASK_STACK_SIZE;
    commandTaskParams.priority = COMMAND_TASK_PRIORITY;
    commandTaskParams.stack = &commandTaskStack;
    Task_construct(&commandTask, commandTaskFunction, &commandTaskParams, NULL);
}


static void commandTaskFunction(UArg arg0, UArg arg1)
{
    int ret;
    while (1)
    {
        ret = messenger_recv(msgBuf);

        /* ignore errors and empty messages
         * first byte is length / 4
         * second byte is opcode
         */
        if (ret < 2) continue;

        switch (msgBuf[1])
        {
        case COMMAND_SETCHANAAPHY:
            if (ret != 12) continue;
            if (msgBuf[2] > 39) continue;
            if (msgBuf[7] > 2) continue;
            setChanAAPHYCRCI(msgBuf[2], *(uint32_t *)(msgBuf + 3),
                    (PHY_Mode)msgBuf[7], *(uint32_t *)(msgBuf + 8));
            break;
        case COMMAND_PAUSEDONE:
            if (ret != 3) continue;
            pauseAfterSniffDone(msgBuf[2] ? true : false);
            break;
        case COMMAND_RSSIFILT:
            if (ret != 3) continue;
            setMinRssi((int8_t)msgBuf[2]);
            break;
        case COMMAND_MACFILT:
            if (ret == 8)
                setMacFilt(true, msgBuf + 2); // filter to supplied MAC
            else
                setMacFilt(false, NULL); // disable MAC filter
            break;
        case COMMAND_ADVHOP:
            if (ret != 2) continue;
            advHopSeekMode();
            break;
        case COMMAND_ENDTRIM:
            if (ret != 6) continue;
            setEndTrim(*(uint32_t *)(msgBuf + 2));
            break;
        case COMMAND_AUXADV:
            if (ret != 3) continue;
            setAuxAdvEnabled(msgBuf[2] ? true : false);
            break;
        default:
            break;
        }
    }
}
