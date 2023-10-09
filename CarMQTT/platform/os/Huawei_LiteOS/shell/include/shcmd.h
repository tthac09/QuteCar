/* ----------------------------------------------------------------------------
 * Copyright (c) Huawei Technologies Co., Ltd. 2013-2018. All rights reserved.
 * Description: shell command
 * Author: liulei
 * Create: 2014-01-01
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written
 * permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --------------------------------------------------------------------------- */
/* ----------------------------------------------------------------------------
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations, which might
 * include those applicable to Huawei LiteOS of U.S. and the country in which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in compliance with such
 * applicable export control laws and regulations.
 * --------------------------------------------------------------------------- */

#ifndef _LITEOS_SHELL_SHCMD_H
#define _LITEOS_SHELL_SHCMD_H

#include "los_list.h"
#include "shcmdparse.h"
#include "los_tables.h"

#ifdef  __cplusplus
#if  __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

typedef BOOL(*ShellVerifyTransIDFunc)(UINT32 transID);

typedef struct {
    CmdType         cmdType;
    CHAR            *cmdKey;
    UINT32          paraNum;
    CmdCallBackFunc cmdHook;
} CmdItem;

typedef struct {
    LOS_DL_LIST list;
    CmdItem *cmdItem;
} CmdItemSet;

typedef struct {
    CmdItemSet  cmdList;
    UINT32      listNum;
    UINT32      initMagicFlag;
    ShellVerifyTransIDFunc transIdHook;
} CmdModInfo;

typedef struct {
    UINT32 cnt;
    LOS_DL_LIST list;
    CHAR cmdString[0];
} CmdKeySave;

#define SHELLCMD_ENTRY(sec, cmdtype, cmdkey, paranum, cmdhook)  \
    CmdItem sec LOS_HAL_TABLE_ENTRY(shellcmd) =                 \
{                                                               \
    cmdtype,                                                    \
    cmdkey,                                                     \
    paranum,                                                    \
    cmdhook                                                     \
};

extern CmdModInfo g_cmdInfo;
extern CmdKeySave g_cmdKeyLink;

UINT32 OsCmdInit(VOID);
UINT32 OsCmdExec(UINT32 transId, CmdParsed *cmdParsed, CHAR *cmdStr);
UINT32 OsCmdKeyShift(const CHAR *cmdKey, CHAR *cmdOut, UINT32 size);
INT32  OsTabCompletion(CHAR *buf, UINT32 *len);
VOID   OsShellCmdPush(const CHAR *str, CmdKeySave *cmdNode);
VOID   OsShellHistoryShow(UINT32 value, CHAR *buf, UINT32 *len, const CmdKeySave *cmdNode);
VOID   OsShellKeyInit(VOID);
VOID   OsShellHistoryReset(UINT32 arg);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif    /* _LITEOS_SHELL_SHCMD_H */
