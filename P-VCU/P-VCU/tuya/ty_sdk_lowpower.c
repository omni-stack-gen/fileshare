/*********************************************************************************
  *Copyright(C),2015-2020, 
  *TUYA 
  *www.tuya.comm
  *FileName:    lowpower demo
**********************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "plog.h"
#include "tuya_iot_config.h"
#include "tuya_ipc_sdk_init.h"
#include "ty_sdk_common.h"


#ifdef ENABLE_DEMO_LOWPOWER

/*
---------------------------------------------------------------------------------
Low power access reference code
en:TRUE is sleep      FALSE is wake
---------------------------------------------------------------------------------
*/
#define PR_TRACE
#define MAXBUF 512

OPERATE_RET TUYA_APP_LOW_POWER_START(CHAR_T *devbuf, CHAR_T *keybuf, TUYA_IP_ADDR_T ip, INT_T port)
{
    INT_T ret=1;
    INT_T i=0;
    INT_T low_power_socket =-1;
    CHAR_T wakeData[36] = {0};
    INT_T wake_data_len = SIZEOF(wakeData);

    while(ret!=0)
    {
        ret = tuya_ipc_low_power_server_connect(ip, port, devbuf, strlen(devbuf), keybuf, strlen(keybuf));
    }

    debug("power_server_connect over.\n");

    while(low_power_socket == -1)
    {
       low_power_socket= tuya_ipc_low_power_socket_fd_get();
    }

    tuya_ipc_low_power_wakeup_data_get(wakeData, &wake_data_len);
    
    debug("wake up date is { ");
    for(i=0;i<wake_data_len;i++)
    {
        debug("0x%x ",wakeData[i]);
    }
    debug(" }\n");

    CHAR_T heart_beat[12] = {0};
    INT_T heart_beat_len = SIZEOF(heart_beat);
    tuya_ipc_low_power_heart_beat_get(heart_beat,&heart_beat_len);
    debug("heart beat data is { ");

    for(i=0;i<heart_beat_len;i++)
    {
        debug("0x%x ",heart_beat[i]);
    }
    debug(" }\n");

    fd_set rfds;
    struct timeval tv;
    INT_T retval, maxfd = -1;

    INT_T len=0;
    CHAR_T recBuf[MAXBUF]={0};
    INT_T heart_timeout=5;
    INT_T user_set_timeout=10;
    while(1)
    {
        FD_ZERO(&rfds);
        FD_SET(0,&rfds);
        maxfd=0;
        FD_SET(low_power_socket,&rfds);
        if (low_power_socket > maxfd)
        {
          maxfd = low_power_socket;
        }
        tv.tv_sec = user_set_timeout;//default 10 seconds;
        tv.tv_usec = 0;

        retval = select(maxfd+1, &rfds, NULL, NULL, &tv);
        if (retval == -1)
        {
          debug("Will exit and the select is error! %s", strerror(errno));
          break;
        }
        else if (retval == 0)
        {
          debug("============send heart beat==============\n");
          len = send(low_power_socket, heart_beat, heart_beat_len, 0);
          if (len < 0)
          {
              debug("socket =%d %d\n",low_power_socket,errno);
              break;
          }
          else
          {
            debug("News: %d \t send, sent a total of %d bytes!\n",
                    heart_beat_len, len);
          }

          continue;
        }
        else
        {
            if (FD_ISSET(low_power_socket, &rfds))
            {
              bzero(recBuf, MAXBUF);
              debug("============recv data==============\n");
              len = recv(low_power_socket, recBuf, MAXBUF, 0);
              if (len > 0)
              {
                  debug("Successfully received the message: is {");
                  for(i=0;i<len;i++)
                      debug("0x%02x ",recBuf[i]);
                  debug("}\n");
                  if(strncmp(recBuf,wakeData,wake_data_len)==0)
                  {
                      //TODO  启动SDK
                      debug("recve data is wake up\n");
                  }

              }
              else
              {
                if (len < 0)
                    debug("Failed to receive the message! \
                          The error code is %d, error message is '%s'\n",
                          errno, strerror(errno));
                else
                    debug("Chat to terminate len=0x%x!\n",len);

                break;
              }
            }

        }
    }

    return 0;
}

#endif
VOID TUYA_IPC_low_power_sample()
{
  TUYA_IP_ADDR_T ip={0};
  int port=0;
  int ret = tuya_ipc_get_low_power_server(&ip, &port);
  if(ret != 0)
  {
      error("get low power ip  error %d\n",ret);
      return;
  }
  #define COMM_LEN 30
  char devid[COMM_LEN]={0};
  int id_len=COMM_LEN;
  ret = tuya_ipc_get_device_id(devid, &id_len);
  if(ret != 0)
  {
      error("get devide error %d\n",ret);
      return;
  }
  char local_key[COMM_LEN]={0};
  int key_len=COMM_LEN;
  ret = tuya_ipc_get_local_key(local_key, &key_len);
  if(ret != 0)
  {
      error("get local key  error %d\n",ret);
      return;
  }
#ifdef ENABLE_DEMO_LOWPOWER
  TUYA_APP_LOW_POWER_START(devid,local_key,ip,port);  
#endif
  return;
}

