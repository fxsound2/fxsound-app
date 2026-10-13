#include "codedefs.h"
#include "GraphicEq.h"
#include "sos.h"
#include "../../dsp/ptutil/DspUtil/GraphicEq/u_GraphicEq.h"
int dspResizeEqSections(void *handle, int bands) {
    auto *eq = static_cast<GraphicEqHdlType *>(handle);
    PT_HANDLE *next = nullptr;
    if (sosNew(&next, eq->slout_hdl, bands) != OKAY) return NOT_OKAY;
    if (sosFreeUp(&eq->sos_hdl) != OKAY) {
        sosFreeUp(&next);
        return NOT_OKAY;
    }
    eq->sos_hdl = next;
    GraphicEqSetMasterGain(reinterpret_cast<PT_HANDLE *>(eq), eq->master_gain);
    GraphicEqSetBalance(reinterpret_cast<PT_HANDLE *>(eq), eq->balance);
    GraphicEqSetNormalization(reinterpret_cast<PT_HANDLE *>(eq), eq->normalization_gain);
    GraphicEqSetVolumeLeveling(reinterpret_cast<PT_HANDLE *>(eq), eq->volume_leveling_gain_db);
    return OKAY;
}
