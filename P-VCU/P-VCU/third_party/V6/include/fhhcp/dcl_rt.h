/**
* @file dcl_rt.h
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef INC_EXTERNAL_DCL_DCL_RT_H_
#define INC_EXTERNAL_DCL_DCL_RT_H_

#include <stdint.h>
#include <stddef.h>
#include "dcl_base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum dclrtRunMode {
    DCL_DEVICE,
    DCL_HOST,
} dclrtRunMode;
    
typedef enum dclrtTsId {
    DCL_TS_ID_AICORE   = 0,
    DCL_TS_ID_AIVECTOR = 1,
    DCL_TS_ID_RESERVED = 2,
} dclrtTsId;
    
typedef enum dclrtEventStatus {
    DCL_EVENT_STATUS_COMPLETE  = 0,
    DCL_EVENT_STATUS_NOT_READY = 1,
    DCL_EVENT_STATUS_RESERVED  = 2,
} dclrtEventStatus;
    
typedef enum dclrtCallbackBlockType {
    DCL_CALLBACK_NO_BLOCK,
    DCL_CALLBACK_BLOCK,
} dclrtCallbackBlockType;
    
typedef enum dclrtMemcpyKind {
    DCL_MEMCPY_HOST_TO_HOST,
    DCL_MEMCPY_HOST_TO_DEVICE,
    DCL_MEMCPY_DEVICE_TO_HOST,
    DCL_MEMCPY_DEVICE_TO_DEVICE,
} dclrtMemcpyKind;

typedef enum dclrtMemMallocPolicy {
    DCL_MEM_MALLOC_HUGE_FIRST,
    DCL_MEM_MALLOC_HUGE_ONLY,
    DCL_MEM_MALLOC_NORMAL_ONLY,
} dclrtMemMallocPolicy;
    
typedef struct rtExceptionInfo dclrtExceptionInfo;
typedef void (*dclrtCallback)(void *userData);
typedef void (*dclrtExceptionInfoCallback)(dclrtExceptionInfo *exceptionInfo);

/**
 * @ingroup DCL
 * @brief Set a callback function to handle exception information
 *
 * @param callback [IN] callback function to handle exception information
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclrtSetExceptionInfoCallback(dclrtExceptionInfoCallback callback);

/**
 * @ingroup DCL
 * @brief Get task id from exception information
 *
 * @param info [IN]   pointer of exception information
 * @retval The task id from exception information
 * @retval 0xFFFFFFFF if info is null
 */
DCL_FUNC_VISIBILITY uint32_t dclrtGetTaskIdFromExceptionInfo(const dclrtExceptionInfo *info);

/**
 * @ingroup DCL
 * @brief Get stream id from exception information
 *
 * @param info [IN]   pointer of exception information
 * @retval The stream id from exception information
 * @retval 0xFFFFFFFF if info is null
 */
DCL_FUNC_VISIBILITY uint32_t dclrtGetStreamIdFromExceptionInfo(const dclrtExceptionInfo *info);

/**
 * @ingroup DCL
 * @brief Get thread id from exception information
 *
 * @param info [IN]   pointer of exception information
 * @retval The thread id of fail task
 * @retval 0xFFFFFFFF if info is null
 */
DCL_FUNC_VISIBILITY uint32_t dclrtGetThreadIdFromExceptionInfo(const dclrtExceptionInfo *info);

/**
 * @ingroup DCL
 * @brief The thread that handles the callback function on the Stream
 *
 * @param threadId [IN]   thread ID
 * @param stream [IN]   stream handle
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclrtSubscribeReport(uint64_t threadId, dclrtStream stream);

/**
 * @ingroup DCL
 * @brief Add a callback function to be executed on the host
 *        to the task queue of the Stream
 *
 * @param fn [IN]   Specify the callback function to be added
 *                  The function prototype of the callback function is:
 *                  typedef void (*dclrtCallback)(void *userData);
 * @param userData [IN]   User data to be passed to the callback function
 * @param blockType [IN]   callback block type
 * @param stream [IN]   stream handle
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclrtLaunchCallback(dclrtCallback fn, void *userData, dclrtCallbackBlockType blockType,
    dclrtStream stream);

/**
 * @ingroup DCL
 * @brief After waiting for a specified time, trigger callback processing
 *
 * @par Function
 *  The thread processing callback specified by
 *  the dclrtSubscribeReport interface
 * @param timeout [IN]   timeout value
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclrtSubscribeReport
 */
DCL_FUNC_VISIBILITY dclError dclrtProcessReport(int32_t timeout);

/**
 * @ingroup DCL
 * @brief Cancel thread registration,
 *        the callback function on the specified Stream
 *        is no longer processed by the specified thread
 *
 * @param threadId [IN]   thread ID
 * @param stream [IN]   stream handle
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclrtUnSubscribeReport(uint64_t threadId, dclrtStream stream);

/**
 * @ingroup DCL
 * @brief create context and associates it with the calling thread
 *
 * @par Function
 * The following use cases are supported:
 * @li If you don't call the dclrtCreateContext interface
 * to explicitly create the context,
 * the system will use the default context, which is implicitly created
 * when the dclrtSetDevice interface is called.
 * @li If multiple contexts are created in a process
 * (there is no limit on the number of contexts),
 * the current thread can only use one of them at the same time.
 * It is recommended to explicitly specify the context of the current thread
 * through the dclrtSetCurrentContext interface to increase.
 * the maintainability of the program.
 * @param  context [OUT]   point to the created context
 * @param  deviceId [IN]    device to create context on
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclrtSetDevice | dclrtSetCurrentContext
 */
DCL_FUNC_VISIBILITY dclError dclrtCreateContext(dclrtContext *context, int32_t deviceId);

/**
 * @ingroup DCL
 * @brief destroy context instance
 *
 * @par Function
 * Can only destroy context created through dclrtCreateContext interface
 * @param  context [IN]   the context to destroy
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclrtCreateContext
 */
DCL_FUNC_VISIBILITY dclError dclrtDestroyContext(dclrtContext context);

/**
 * @ingroup DCL
 * @brief set the context of the thread
 *
 * @par Function
 * The following scenarios are supported:
 * @li If the dclrtCreateContext interface is called in a thread to explicitly
 * create a Context (for example: ctx1), the thread's Context can be specified
 * without calling the dclrtSetCurrentContext interface.
 * The system uses ctx1 as the context of thread1 by default.
 * @li If the dclrtCreateContext interface is not explicitly created,
 * the system uses the default context as the context of the thread.
 * At this time, the dclrtDestroyContext interface cannot be used to release
 * the default context.
 * @li If the dclrtSetCurrentContext interface is called multiple times to
 * set the thread's Context, the last one prevails.
 *
 * @par Restriction
 * @li If the cevice corresponding to the context set for the thread
 * has been reset, you cannot set the context as the context of the thread,
 * otherwise a business exception will result.
 * @li It is recommended to use the context created in a thread.
 * If the dclrtCreateContext interface is called in thread A to create a context,
 * and the context is used in thread B,
 * the user must guarantee the execution order of tasks in the same stream
 * under the same context in two threads.
 * @param  context [IN]   the current context of the thread
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclrtCreateContext | dclrtDestroyContext
 */
DCL_FUNC_VISIBILITY dclError dclrtSetCurrentContext(dclrtContext context);

/**
 * @ingroup DCL
 * @brief get the context of the thread
 *
 * @par Function
 * If the user calls the dclrtSetCurrentContext interface
 * multiple times to set the context of the current thread,
 * then the last set context is obtained
 * @param  context [OUT]   the current context of the thread
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclrtSetCurrentContext
 */
DCL_FUNC_VISIBILITY dclError dclrtGetCurrentContext(dclrtContext *context);

/**
 * @ingroup DCL
 * @brief Specify the device to use for the operation
 * implicitly create the default context and the default stream
 *
 * @par Function
 * The following use cases are supported:
 * @li Device can be specified in the process or thread.
 * If you call the dclrtSetDevice interface multiple
 * times to specify the same device,
 * you only need to call the dclrtResetDevice interface to reset the device.
 * @li The same device can be specified for operation
 *  in different processes or threads.
 * @li Device is specified in a process,
 * and multiple threads in the process can share this device to explicitly
 * create a Context (dclrtCreateContext interface).
 * @li In multi-device scenarios, you can switch to other devices
 * through the dclrtSetDevice interface in the process.
 * @param  deviceId [IN]  the device id
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclrtResetDevice |dclrtCreateContext
 */
DCL_FUNC_VISIBILITY dclError dclrtSetDevice(int32_t deviceId);

/**
 * @ingroup DCL
 * @brief Reset the current operating Device and free resources on the device,
 * including the default context, the default stream,
 * and all streams created under the default context,
 * and synchronizes the interface.
 * If the task under the default context or stream has not been completed,
 * the system will wait for the task to complete before releasing it.
 *
 * @par Restriction
 * @li The Context, Stream, and Event that are explicitly created
 * on the device to be reset. Before resetting,
 * it is recommended to follow the following interface calling sequence,
 * otherwise business abnormalities may be caused.
 * @li Interface calling sequence:
 * call dclrtDestroyEvent interface to release Event or
 * call dclrtDestroyStream interface to release explicitly created Stream->
 * call dclrtDestroyContext to release explicitly created Context->
 * call dclrtResetDevice interface
 * @param  deviceId [IN]   the device id
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclrtResetDevice(int32_t deviceId);

/**
 * @ingroup DCL
 * @brief get target device of current thread
 *
 * @param deviceId [OUT]  the device id
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclrtGetDevice(int32_t *deviceId);

/**
 * @ingroup DCL
 * @brief get target side
 *
 * @param runMode [OUT]    the run mode
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclrtGetRunMode(dclrtRunMode *runMode);

/**
 * @ingroup DCL
 * @brief Wait for compute device to finish
 *
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclrtSynchronizeDevice(void);

/**
 * @ingroup DCL
 * @brief Set Scheduling TS
 *
 * @param tsId [IN]   the ts id
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclrtSetTsDevice(dclrtTsId tsId);

/**
 * @ingroup DCL
 * @brief get total device number.
 *
 * @param count [IN|OUT]    the device number
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclrtGetDeviceCount(uint32_t *count);

/**
 * @ingroup DCL
 * @brief create event instance
 *
 * @param event [OUT]   created event
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclrtCreateEvent(dclrtEvent *event);

/**
 * @ingroup DCL
 * @brief destroy event instance
 *
 * @par Function
 *  Only events created through the dclrtCreateEvent interface can be
 *  destroyed, synchronous interfaces. When destroying an event,
 *  the user must ensure that the tasks involved in the dclrtSynchronizeEvent
 *  interface or the dclrtStreamWaitEvent interface are completed before
 *  they are destroyed.
 * @param  event [IN]   event to destroy
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclrtCreateEvent | dclrtSynchronizeEvent | dclrtStreamWaitEvent
 */
DCL_FUNC_VISIBILITY dclError dclrtDestroyEvent(dclrtEvent event);

/**
 * @ingroup DCL
 * @brief Record an Event in the Stream
 *
 * @param event [IN]   event to record
 * @param stream [IN]   stream handle
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclrtRecordEvent(dclrtEvent event, dclrtStream stream);

/**
 * @ingroup DCL
 * @brief Reset an event
 *
 * @par Function
 *  Users need to make sure to wait for the tasks in the Stream
 *  to complete before resetting the Event
 * @param event [IN]   event to reset
 * @param stream [IN]   stream handle
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclrtResetEvent(dclrtEvent event, dclrtStream stream);

/**
 * @ingroup DCL
 * @brief Queries an event's status
 *
 * @param  event [IN]  event to query
 * @param  status [OUT]  event status
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclrtQueryEvent(dclrtEvent event, dclrtEventStatus *status);

/**
 * @ingroup DCL
 * @brief Block Host Running, wait event to be complete
 *
 * @param  event [IN]   event to wait
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclrtSynchronizeEvent(dclrtEvent event);

/**
 * @ingroup DCL
 * @brief computes the elapsed time between events.
 *
 * @param ms [OUT]    time between start and end in ms
 * @param start [IN]    starting event
 * @param end [IN]     ending event
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclrtCreateEvent | dclrtRecordEvent | dclrtSynchronizeStream
 */
DCL_FUNC_VISIBILITY dclError dclrtEventElapsedTime(float *ms, dclrtEvent start, dclrtEvent end);

/**
 * @ingroup DCL
 * @brief synchronous memory replication between host and device
 *
 * @param dst [IN]     destination address pointer
 * @param destMax [IN]   Max length of the destination address memory
 * @param src [IN]     source address pointer
 * @param count [IN]   the length of byte to copy
 * @param kind [IN]    memcpy type
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclrtMemcpy(void *dst,
                                         size_t destMax,
                                         const void *src,
                                         size_t count,
                                         dclrtMemcpyKind kind);

/**
 * @ingroup DCL
 * @brief Initialize memory and set contents of memory to specified value
 *
 * @par Function
 *  The memory to be initialized is on the Host or device side,
 *  and the system determines whether
 *  it is host or device according to the address
 * @param devPtr [IN]    Starting address of memory
 * @param maxCount [IN]  Max length of destination address memory
 * @param value [IN]     Set value
 * @param count [IN]     The length of memory
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclrtMemset(void *devPtr, size_t maxCount, int32_t value, size_t count);

/**
 * @ingroup DCL
 * @brief  Asynchronous memory replication between Host and Device
 *
 * @par Function
 *  After calling this interface,
 *  be sure to call the dclrtSynchronizeStream interface to ensure that
 *  the task of memory replication has been completed
 *
 * @par Restriction
 * @li For on-chip Device-to-Device memory copy,
 *     both the source and destination addresses must be 64-byte aligned
 * @param dst [IN]     destination address pointer
 * @param destMax [IN] Max length of destination address memory
 * @param src [IN]     source address pointer
 * @param count [IN]   the number of byte to copy
 * @param kind [IN]    memcpy type
 * @param stream [IN]   asynchronized task stream
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclrtSynchronizeStream
 */
DCL_FUNC_VISIBILITY dclError dclrtMemcpyAsync(void *dst,
                                              size_t destMax,
                                              const void *src,
                                              size_t count,
                                              dclrtMemcpyKind kind,
                                              dclrtStream stream);

/**
* @ingroup DCL
* @brief Asynchronous initialize memory
* and set contents of memory to specified value async
*
* @par Function
 *  The memory to be initialized is on the Host or device side,
 *  and the system determines whether
 *  it is host or device according to the address
* @param devPtr [IN]      destination address pointer
* @param maxCount [IN]    Max length of destination address memory
* @param value [IN]      set value
* @param count [IN]      the number of byte to set
* @param stream [IN]     asynchronized task stream
* @retval DCL_ERROR_NONE The function is successfully executed.
* @retval OtherValues Failure
*
* @see dclrtSynchronizeStream
*/
DCL_FUNC_VISIBILITY dclError dclrtMemsetAsync(void *devPtr,
                                              size_t maxCount,
                                              int32_t value,
                                              size_t count,
                                              dclrtStream stream);

/**
 * @ingroup DCL
 * @brief  create stream instance
 *
 * @param  stream [OUT]   the created stream
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclrtCreateStream(dclrtStream *stream);

/**
 * @ingroup DCL
 * @brief destroy stream instance
 *
 * @par Function
 * Can only destroy streams created through the dclrtCreateStream interface
 *
 * @par Restriction
 * Before calling the dclrtDestroyStream interface to destroy
 * the specified Stream, you need to call the dclrtSynchronizeStream interface
 * to ensure that the tasks in the Stream have been completed.
 * @param stream [IN]  the stream to destroy
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclrtCreateStream | dclrtSynchronizeStream
 */
DCL_FUNC_VISIBILITY dclError dclrtDestroyStream(dclrtStream stream);

/**
 * @ingroup DCL
 * @brief block the host until all tasks
 * in the specified stream have completed
 *
 * @param  stream [IN]   the stream to wait
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclrtSynchronizeStream(dclrtStream stream);

/**
 * @ingroup DCL
 * @brief Blocks the operation of the specified Stream until
 * the specified Event is completed.
 * Support for multiple streams waiting for the same event.
 *
 * @param  stream [IN]   the wait stream If using thedefault Stream, set NULL
 * @param  event [IN]    the event to wait
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclrtStreamWaitEvent(dclrtStream stream, dclrtEvent event);

#ifdef __cplusplus
}
#endif

#endif // INC_EXTERNAL_DCL_DCL_RT_H_

