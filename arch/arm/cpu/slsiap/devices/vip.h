#ifndef _NXP_VIP_H_
#define _NXP_VIP_H_

struct nxp_vip_param {
    //int  module;
    int  port;
    bool external_sync;
    bool is_mipi;

    u32  h_active;
    u32  h_frontporch;
    u32  h_syncwidth;
    u32  h_backporch;
    u32  v_active;
    u32  v_frontporch;
    u32  v_syncwidth;
    u32  v_backporch;
    u32  data_order;
    bool interlace;
};

int nxp_vip_register_param(int module, struct nxp_vip_param *param);
int nxp_vip_set_addr(int module, u32 lu_addr, u32 cb_addr, u32 cr_addr);
int nxp_vip_run(int module);

#endif
