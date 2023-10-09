/*----------------------------------------------------------------------------
* Copyright (c) <2013-2015>, <Huawei Technologies Co., Ltd>
* All rights reserved.
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
*---------------------------------------------------------------------------*/

#ifndef __LinkServiceAgent__H__
#define __LinkServiceAgent__H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LinkServiceAgent {
    /*
     * this funtion to get device's property value.
     * s : ServiceAgent
     * property : device's property name
     * value : a json string of device's property value
     * len : value's length
     * rerurn code:
     * StatusOk : get value successfully
     * StatusFailure : failure to get value
     * StatusDeviceOfffline : device had offline
     */
    int (*get)(struct LinkServiceAgent* s, const char* property, char* value, int len);
    /*
     * this funtion to modify device's property
     * s : ServiceAgent
     * property : device's property name
     * value : a json string of device's property
     * len : value's length
     * rerurn code:
     * StatusOk : moified value successfully
     * StatusFailure : failure to moified value
     * StatusDeviceOfffline : device had offline
     */
    int (*modify)(struct LinkServiceAgent* s, const char* property, char* value, int len);
    /*
     * get device's service type
     * return code:
     * if type is not null, then return a string,otherwise,
     * a null string return
     */
    const char* (*type)(struct LinkServiceAgent* s);
    /*
     * get address of device's service
     */
    const char* (*addr)(struct LinkServiceAgent* s);
    /*
     * the id of device's service
     */
    int (*id)(struct LinkServiceAgent* s);
} LinkServiceAgent;

/*
 * free ServiceAgent that malloced by LinkAgent
 */
void LinkServiceAgentFree(LinkServiceAgent* s);

/// for C++
#ifdef __cplusplus
}
#endif

#endif /*__LinkServiceAgent__H__*/
