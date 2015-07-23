#ifndef _NXP_MLC_H_
#define _NXP_MLC_H_

struct nxp_mlc_video_param {
    u32 format;
    u32 width;
    u32 height;
    u32 left;
    u32 top;
    u32 right;
    u32 bottom;
    u32 priority;
    u32 colorkey;
    u32 alpha;
    u32 brightness;
    u32 hue;
};

void nxp_mlc_video_set_param(int module, struct nxp_mlc_video_param *param);
void nxp_mlc_video_set_addr(int module, u32 lu_a, u32 cb_a, u32 cr_a, u32 lu_s, u32 cb_s, u32 cr_s);
void nxp_mlc_video_run(int module);

#endif
