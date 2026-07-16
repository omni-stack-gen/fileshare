#include "sample_common.h"
// #include "chn_bind_type.h"
// #include "bind_struct_info.h"
#include "isp/isp_common.h"
#include "dsp/fh_system_mpi.h"

extern FH_SINT32 sample_common_vpu_bind_to_jpege(FH_UINT32 grpid, FH_UINT32 chan_vpu, FH_UINT32 chan_jpege);
extern FH_SINT32 sample_common_vpu_bind_to_enc(FH_UINT32 grpid, FH_UINT32 chan_vpu, FH_UINT32 chan_enc);
extern FH_SINT32 sample_common_vpu_unbind_to_enc(FH_UINT32 grpid, FH_UINT32 chan_vpu, FH_UINT32 chan_enc);

FH_SINT32 sample_common_start_bind(FH_VOID)
{
    int ret;
    int grploop=0, chnloop=0;


    // struct grp_vpu_info grp_info = {0};
    // struct vpu_channel_info vpu_info = {0};
    // struct enc_channel_info enc_info = {0};

    //struct bind_dev_info *bind_info;
    FH_BIND_INFO src, dst;
    ///struct dev_isp_info isp_info = {0};

    //for (grploop = 0; grploop < MAX_GRP_NUM; grploop++)
    {
        //sample_common_dsp_get_grp_info(grploop, &grp_info);
        //printf("######grploop %d, enable %d\n", grploop, grp_info.enable);
        //sample_common_get_isp_info(grploop, &isp_info);

        {
            src.obj_id = FH_OBJ_ISP;
            src.dev_id = grploop;
            {
                src.chn_id = 0;
            }

            dst.obj_id = FH_OBJ_VPU_VI;
            dst.dev_id = 0;//grp_info.channel;
            dst.chn_id = 0;
            ret = FH_SYS_Bind(src, dst);
            if (ret != 0)
            {
                //printf("Error(%d - %x): FH_SYS_Bind ISP[%d] to VPU[%d]-chn[%d]\n", ret, ret, isp_info.channel, dst.dev_id, dst.chn_id);
                return -1;
            }
            //printf("FH_SYS_Bind ISP[%d] to VPU[%d]-chn[%d]\n", isp_info.channel, dst.dev_id, dst.chn_id);
        }

        for (chnloop = 0; chnloop < ENC_CHANNEL_NUM; chnloop++)
        {
            // sample_common_dsp_get_vpu_chn_info(grploop, chnloop, &vpu_info);
            // sample_common_dsp_get_enc_chn_info(grploop, chnloop, &enc_info);
            //bind_info = &g_bind_dev_info[grploop * MAX_VPU_CHN_NUM + chnloop];

            //printf("chnloop :%d enc_info.enable :%d vpu_info.channel:%d enc_info.channel:%d\n", chnloop, enc_info.enable, vpu_info.channel, enc_info.channel);
            //if (enc_info.enable)
            {
                //if (bind_info->vpu2 == FH_OBJ_ENC)
                //{
                    //ret = sample_common_vpu_bind_to_enc(grp_info.channel, vpu_info.channel, enc_info.channel);
                    ret = sample_common_vpu_bind_to_enc(0, chnloop, chnloop);
                    if (ret != 0)
                    {
                        //printf("Error(%d - %x): FH_SYS_Bind VPU to ENC([%d][%d] to %d)\n", ret, ret, grp_info.channel, vpu_info.channel, enc_info.channel);
                        return -1;
                    }
                // } else if(bind_info->vpu2 == FH_OBJ_JPEG) {
                //     ret = sample_common_vpu_bind_to_jpege(grp_info.channel, vpu_info.channel, enc_info.channel);
                //     if (ret != 0)
                //     {
                //         printf("Error(%d - %x): FH_SYS_Bind VPU to ENC([%d][%d] to %d)\n", ret, ret, grp_info.channel, vpu_info.channel, enc_info.channel);
                //         return -1;
                //     }
                // }
            }
        }
    }
    return 0;
}


FH_SINT32 sample_common_stop_bind(FH_VOID)
{
		int ret;
		int grploop, chnloop;

		// struct grp_vpu_info grp_info;
		// struct vpu_channel_info vpu_info;
		// struct enc_channel_info enc_info;
		// struct bind_dev_info *bind_info;

		//for (grploop = 0; grploop < MAX_GRP_NUM; grploop++)
		//{
			//sample_common_dsp_get_grp_info(grploop, &grp_info);
            grploop = 0;
			for (chnloop = 0; chnloop < ENC_CHANNEL_NUM; chnloop++)
			{
				//sample_common_dsp_get_vpu_chn_info(grploop, chnloop, &vpu_info);
				//sample_common_dsp_get_enc_chn_info(grploop, chnloop, &enc_info);
				//bind_info = &g_bind_dev_info[grploop * MAX_VPU_CHN_NUM + chnloop];

				// if (enc_info.enable)
				// {
					// if (bind_info->vpu2 == FH_OBJ_ENC)
					// {
						ret = sample_common_vpu_unbind_to_enc(grploop, chnloop, chnloop);//sample_common_vpu_unbind_to_enc(grp_info.channel, vpu_info.channel, enc_info.channel);
						if (ret != 0)
						{
							//printf("Error(%d - %x): FH_SYS_Bind VPU to ENC([%d][%d] to %d)\n", ret, ret, grp_info.channel, vpu_info.channel, enc_info.channel);
							return -1;
						}
					// } else if(bind_info->vpu2 == FH_OBJ_JPEG) {
                    // ret = sample_common_vpu_unbind_to_jpege(grp_info.channel, vpu_info.channel, enc_info.channel);
                    // if (ret != 0)
                    // {
                    //     printf("Error(%d - %x): FH_SYS_Bind VPU to jpege([%d][%d] to %d)\n", ret, ret, grp_info.channel, vpu_info.channel, enc_info.channel);
                    //     return -1;
                    // }
                //}
				//}
			}
		//}

    return 0;
}

