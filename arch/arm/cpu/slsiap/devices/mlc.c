#include <common.h>
#include <platform.h>
#include <asm/arch/display.h>

#include "mlc.h"

#ifndef MLC_LAYER_VIDEO
#define MLC_LAYER_VIDEO     3
#endif

void nxp_mlc_video_set_param(int module, struct nxp_mlc_video_param *param)
{
    int srcw = param->width;
    int srch = param->height;
    int dstw, dsth, hf, vf;

    if (param->right == 0)
        NX_MLC_GetScreenSize(module, &param->right, &param->bottom);

    dstw = param->right - param->left;
    dsth = param->bottom - param->top;
    hf = 1, vf = 1;

    if (srcw == dstw && srch == dsth)
        hf = 0, vf = 0;

    NX_MLC_SetFormatYUV(module, param->format);
    NX_MLC_SetVideoLayerScale(module, srcw, srch, dstw, dsth,
            (CBOOL)hf, (CBOOL)hf, (CBOOL)vf, (CBOOL)vf);
    NX_MLC_SetPosition(module, MLC_LAYER_VIDEO,
            param->left, param->top, param->right - 1, param->bottom - 1);
    NX_MLC_SetLayerPriority(module, param->priority);
    NX_MLC_SetDirtyFlag(module, MLC_LAYER_VIDEO);
    printf("%s exit\n", __func__);
}

void nxp_mlc_video_set_addr(int module, u32 lu_a, u32 cb_a, u32 cr_a, u32 lu_s, u32 cb_s, u32 cr_s)
{
    NX_MLC_SetVideoLayerStride (module, lu_s, cb_s, cr_s);
    NX_MLC_SetVideoLayerAddress(module, lu_a, cb_a, cr_a);
    NX_MLC_SetVideoLayerLineBufferPowerMode(module, CTRUE);
    NX_MLC_SetVideoLayerLineBufferSleepMode(module, CFALSE);
    NX_MLC_SetDirtyFlag(module, MLC_LAYER_VIDEO);
    printf("%s exit\n", __func__);
}

void nxp_mlc_video_run(int module)
{
	NX_MLC_SetTopDirtyFlag(module);
    NX_MLC_SetLayerEnable(module, MLC_LAYER_VIDEO, CTRUE);
    NX_MLC_SetDirtyFlag(module, MLC_LAYER_VIDEO);
    printf("%s exit\n", __func__);
}
