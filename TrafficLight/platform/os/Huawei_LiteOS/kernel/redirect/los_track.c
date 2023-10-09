/* ----------------------------------------------------------------------------
 * Copyright (c) Huawei Technologies Co., Ltd. 2013-2018. All rights reserved.
 * Description: TRACK
 * Author: jianghan
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
#include "los_track.h"
#include "los_memory_pri.h"
#include "los_sys_pri.h"
#include "securec.h"
#include "los_hwi.h"
#include "stdint.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#if (LOSCFG_BASE_MISC_TRACK == YES)

#define LOS_TRACK_MGR_SIZE     sizeof(TRACK_MGR_S)

UINT16             g_trackMask    = 0;
USER_CUR_US_FUNC   g_curUsGetHook = NULL;
static TRACK_MGR_S *g_trackMgr    = NULL;

/*****************************************************************************
Function   : OsTrackhook
Description: Defalut track hook function
Input      :  usTrackType :The  type of track item.
            uwTrackId   :The   id  of track item.
               uwUserData: The  user data of track item.
               uwEntry:      The  entry of function .
Output     : None
Return     : None
*****************************************************************************/
LITE_OS_RAM_SECTION VOID OsTrackhook(UINT16 trackType, UINT16 trackId, UINT32 userData, UINT32 entry)
{
    UINT16 index;
    UINT32 intSave;
    TRACK_ITEM_S *trackRunItem = NULL;
    if (trackType & g_trackMask) {
        intSave = LOS_IntLock();
        index = g_trackMgr->usCurIndex;
        trackRunItem = &g_trackMgr->pstTrackItems[index];
        trackRunItem->usTrackType = trackType;
        trackRunItem->usData0 = trackId;
        trackRunItem->uwData1 = userData;
        trackRunItem->uwEntry = entry;

        if (g_curUsGetHook != NULL) {
            (VOID)g_curUsGetHook(&(trackRunItem->uwCurTime));
        } else {
            trackRunItem->uwCurTime = (UINT32)LOS_TickCountGet();
        }

        g_trackMgr->usCurIndex++;
        if (g_trackMgr->usCurIndex >= g_trackMgr->usMaxRecordCount) {
            g_trackMgr->usCurIndex = 0;
        }
        LOS_IntRestore(intSave);
    }
}

LITE_OS_SEC_TEXT_INIT_REDIRECT UINT32 LOS_CurUsRegHook(USER_CUR_US_FUNC hook)
{
    UINTPTR intSave;
    intSave = LOS_IntLock();
    g_curUsGetHook = hook;
    LOS_IntRestore(intSave);
    return LOS_OK;
}

LITE_OS_SEC_TEXT_INIT_REDIRECT UINT32 LOS_TrackRegHook(TRACK_PROC_HOOK hook)
{
    UINTPTR intSave;
    UINT32 ret;
    ret = LOS_TrackDelete();
    if ((ret != LOS_ERRNO_TRACK_NO_INIT) && (ret != LOS_OK)) {
        return ret;
    }
    intSave = LOS_IntLock();
    g_trackHook = hook;
    LOS_IntRestore(intSave);
    return LOS_OK;
}

/*****************************************************************************
Function   : LOS_TrackInit
Description: Initializes memory of track manager and track items
Input      : maxRecordCount: The max track count of track items.
Output     : None
Return     : LOS_OK on success or error code on failure
*****************************************************************************/
LITE_OS_SEC_TEXT_INIT_REDIRECT UINT32 LOS_TrackInit(UINT16 maxRecordCount)
{
    UINT32 size;
    UINT32 intSave;
    VOID   *buf = NULL;

    if (maxRecordCount > LOSCFG_BASE_MISC_TRACK_MAX_COUNT) {
        return LOS_ERRNO_TRACK_TOO_LARGE_COUNT;
    }

    if (maxRecordCount == 0) {
        return LOS_ERRNO_TRACK_PARA_ISZERO;
    }

    intSave = LOS_IntLock();
    if (g_trackMgr != NULL) {
        LOS_IntRestore(intSave);
        return LOS_ERRNO_TRACK_RE_INIT;
    }
    size = LOS_TRACK_MGR_SIZE + sizeof(TRACK_ITEM_S) * maxRecordCount;

    buf = LOS_MemAlloc(m_aucSysMem0, size);
    if (buf == NULL) {
        LOS_IntRestore(intSave);
        return LOS_ERRNO_TRACK_NO_MEMORY;
    }
    (VOID)memset_s(buf, size, 0, size);
    g_trackMgr = (TRACK_MGR_S *)buf;
    g_trackMgr->pstTrackItems = (TRACK_ITEM_S *)((uintptr_t)buf + LOS_TRACK_MGR_SIZE);
    g_trackMgr->usMaxRecordCount = maxRecordCount;
    g_trackMgr->usCurIndex = 0;

    LOS_IntRestore(intSave);
    return LOS_OK;
}

/*****************************************************************************
Function   : LOS_TrackStart
Description: Start recording the track
Input      : trackMask: The mask of information needed to track.
Output     : None
Return     : LOS_OK on success or error code on failure
*****************************************************************************/
LITE_OS_SEC_TEXT_INIT_REDIRECT UINT32 LOS_TrackStart(UINT16 trackMask)
{
    UINT32 intSave;
    intSave = LOS_IntLock();
    if (g_trackMgr == NULL) {
        LOS_IntRestore(intSave);
        return LOS_ERRNO_TRACK_NO_INIT;
    }
    g_trackMask = trackMask;
    LOS_IntRestore(intSave);
    return LOS_OK;
}

/*****************************************************************************
Function   : LOS_TrackStop
Description: Stop recording track
Input      : None
Output     : None
Return     : maskSave: Save the mask of information needed to track.
*****************************************************************************/
LITE_OS_SEC_TEXT_INIT_REDIRECT UINT16 LOS_TrackStop(VOID)
{
    UINT32 intSave;
    UINT16 maskSave;
    maskSave = g_trackMask;
    intSave = LOS_IntLock();
    g_trackMask = 0;
    LOS_IntRestore(intSave);
    return maskSave;
}

/*****************************************************************************
Function   : LOS_TrackInfoGet
Description: Get track manager info
Input      : None
Output     : info: Pointer to the track manager information structure to be obtained.
Return     : LOS_OK on success or error code on failure
*****************************************************************************/
LITE_OS_SEC_TEXT_INIT_REDIRECT UINT32 LOS_TrackInfoGet(TRACK_MGR_S *info)
{
    UINT32 intSave;
    if (info == NULL) {
        return LOS_ERRNO_TRACK_PTR_NULL;
    }
    intSave = LOS_IntLock();
    if (g_trackMgr == NULL) {
        LOS_IntRestore(intSave);
        return LOS_ERRNO_TRACK_NO_INIT;
    }

    info->usCurIndex = g_trackMgr->usCurIndex;
    info->usMaxRecordCount = g_trackMgr->usMaxRecordCount;
    info->pstTrackItems = g_trackMgr->pstTrackItems;
    LOS_IntRestore(intSave);
    return LOS_OK;
}

/*****************************************************************************
Function   : LOS_TrackItemGetById
Description: Get track item info by index
Input      : index:    The index of track items.
Output     : itemInfo: Pointer to the track item information structure to be obtained.
Return     : LOS_OK on success or error code on failure
*****************************************************************************/
LITE_OS_SEC_TEXT_INIT_REDIRECT UINT32 LOS_TrackItemGetById(UINT16 index, TRACK_ITEM_S *itemInfo)
{
    UINT32       intSave;
    TRACK_ITEM_S *trackItem = NULL;

    if (itemInfo == NULL) {
        return LOS_ERRNO_TRACK_PTR_NULL;
    }

    intSave = LOS_IntLock();
    if (g_trackMgr == NULL) {
        LOS_IntRestore(intSave);
        return LOS_ERRNO_TRACK_NO_INIT;
    }

    if (index >= g_trackMgr->usMaxRecordCount) {
        LOS_IntRestore(intSave);
        return LOS_ERRNO_TRACK_INVALID_PARAM;
    }

    trackItem = &g_trackMgr->pstTrackItems[index];
    (VOID)memcpy_s(itemInfo, sizeof(TRACK_ITEM_S), trackItem, sizeof(TRACK_ITEM_S));
    LOS_IntRestore(intSave);
    return LOS_OK;
}

/*****************************************************************************
Function   : LOS_TrackDelete
Description: Delete the track
Input      : None
Output     : None
Return     : LOS_OK on success or error code on failure
*****************************************************************************/
LITE_OS_SEC_TEXT_INIT_REDIRECT UINT32 LOS_TrackDelete(VOID)
{
    UINT32 ret;
    UINT32 intSave;

    intSave = LOS_IntLock();
    if (g_trackMgr == NULL) {
        LOS_IntRestore(intSave);
        return LOS_ERRNO_TRACK_NO_INIT;
    }
    g_trackMask = 0;
    ret = LOS_MemFree(m_aucSysMem0, (VOID *)g_trackMgr);
    if (ret != LOS_OK) {
        LOS_IntRestore(intSave);
        return ret;
    }
    g_trackMgr = NULL;
    LOS_IntRestore(intSave);
    return LOS_OK;
}

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
