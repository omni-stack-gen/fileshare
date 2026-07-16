
/**
* @file dcl_mpi.h
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef INC_EXTERNAL_DCL_MPI_H
#define INC_EXTERNAL_DCL_MPI_H


#ifdef __cplusplus
extern "C" {
#endif

#include "dcl_rt.h"
#include "dcl_mpi_vdec.h"
#include "dcl_mpi_venc.h"
#include "dcl_mpi_vpc.h"

typedef enum {
    DCL_MPI_EPOLL_IN = 1u,
    DCL_MPI_EPOLL_OUT = 1u << 1,
    DCL_MPI_EPOLL_ET = 1u << 31
} dclMpiEpollEventType;

typedef enum {
    DCL_MPI_EPOLL_CTL_ADD = 1,
    DCL_MPI_EPOLL_CTL_MOD = 2,
    DCL_MPI_EPOLL_CTL_DEL = 3,
    DCL_MPI_EPOLL_CTL_BUTT
} dclMpiEpollCtlOp;

typedef struct {
    uint32_t events; // bitmask of dclMpiEpollEventType
    void    *data;   // usrdata
} dclEpollEvent;

/**
 * @ingroup DCL
 * @brief Media Platform Interface initialize 
 *
 * @param  
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see 
 */
DCL_FUNC_VISIBILITY dclError dclmpiInit(void);


/**
 * @ingroup DCL
 * @brief Media Platform Interface exit 
 *
 * @param  
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see 
 */
DCL_FUNC_VISIBILITY dclError dclmpiExit(void);


/**
 * @ingroup DCL
 * @brief Query version information of MPI
 *
 * @param  majorVersion   [OUT]  
 * @param  minorVersion   [OUT]    
 * @param  patchVersion   [OUT]  
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see 
 */
DCL_FUNC_VISIBILITY dclError dclmpiGetVersion(int32_t *majorVersion, 
               int32_t *minorVersion, int32_t *patchVersion);


/**
 * @ingroup DCL
 * @brief alloc memory on device
 *
 * @param  devPtr    [OUT]  pointer to pointer to allocated memory on device
 * @param  size      [IN]   alloc memory size
 * @param  policy    [IN]   memory alloc policy
 * @param  align     [IN]   align value of malloc
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see 
 */
DCL_FUNC_VISIBILITY dclError dclmpiMalloc(void **devPtr, uint64_t size, dclrtMemMallocPolicy policy, uint32_t align);


/**
 * @ingroup DCL
 * @brief free device memory
 *
 * @par Function
 *  can only free memory allocated through the dclmpiMalloc interface
 * @param  devPtr [IN]  Pointer to memory to be freed
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmpiMalloc
 */
DCL_FUNC_VISIBILITY dclError dclmpiFree(void *devPtr);


/**
 * @ingroup DCL
 * @brief create epoll 
 *
 * @param  size      [IN]   size of fd for epoll
 * @param  epollFd   [OUT]  epoll instance 
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see 
 */
DCL_FUNC_VISIBILITY dclError dclmpiCreateEpoll(uint32_t size, int32_t *epollFd);

/**
 * @ingroup DCL
 * @brief control epoll 
 *
 * @param  epollFd   [IN]   epoll instance 
 * @param  operation [IN]   operation type: add, delete, modify fd of epoll
 * @param  fd        [IN]   file handle to be monitored
 * @param  event     [IN]   monitored poll event
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmpiCreateEpoll
 */
DCL_FUNC_VISIBILITY dclError dclmpiCtrlEpoll(int32_t epollFd, int32_t operation, int32_t fd, dclEpollEvent *event);

/**
 * @ingroup DCL
 * @brief control epoll 
 *
 * @param  epollFd   [IN]   epoll instance 
 * @param  event     [OUT]  happened poll event 
 * @param  maxEvents [IN]   max event num can be saved
 * @param  timeOut   [IN]   -1: block, 0:non-block, >0: wait time
 * @param  eventNum  [OUT]  event num happened
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see 
 */
DCL_FUNC_VISIBILITY dclError dclmpiWaitEpoll(int32_t epollFd, dclEpollEvent *event, 
                    int32_t maxEvents, int32_t timeOut, int32_t *eventNum);

/**
 * @ingroup DCL
 * @brief close epoll 
 *
 * @param  epollFd   [IN]  epoll instance 
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmpiCreateEpoll
 */
DCL_FUNC_VISIBILITY dclError  dclmpiCloseEpoll(int32_t epollFd);

#ifdef __cplusplus
}
#endif


#endif
