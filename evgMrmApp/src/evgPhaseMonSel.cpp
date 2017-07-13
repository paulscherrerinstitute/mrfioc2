#include "evgPhaseMonSel.h"

#include "evgRegMap.h"
#include "mrfCommonIO.h"

#include <stdexcept>

evgPhaseMonSel::evgPhaseMonSel(const std::string& name, volatile epicsUInt8*
        const pPhReg)
: mrf::ObjectInst<evgPhaseMonSel>(name)
, m_pPhReg(pPhReg) {
}

evgPhaseMonSel::~evgPhaseMonSel() {
}

epicsUInt16 evgPhaseMonSel::getRiEdgeRaw() const {
    return (epicsUInt16) getRiEdge();
}

epicsUInt16 evgPhaseMonSel::getFaEdgeRaw() const {
    return (epicsUInt16) getFaEdge();
}

evgPhaseMonSel::PhMonRi evgPhaseMonSel::getRiEdge() const {
    epicsUInt32 reg;

    reg = nat_ioread32(m_pPhReg);

    reg &= EVG_FPInPhMon_PHRE_mask;
    reg >>= EVG_FPInPhMon_PHRE_shift;

    switch (reg) {
    case PhMonRi_reset:
    case PhMonRi_0to90:
    case PhMonRi_90to180:
    case PhMonRi_180to270:
    case PhMonRi_270to0:
        break;
    default:
        reg = PhMonRi_invalid;
        break;
    }

    return (PhMonRi) reg;
}

evgPhaseMonSel::PhMonFa evgPhaseMonSel::getFaEdge() const {
    epicsUInt32 reg;

    reg = nat_ioread32(m_pPhReg);

    reg &= EVG_FPInPhMon_PHFE_mask;
    reg >>= EVG_FPInPhMon_PHFE_shift;

    switch (reg) {
    case PhMonFa_reset:
    case PhMonFa_0to90:
    case PhMonFa_90to180:
    case PhMonFa_180to270:
    case PhMonFa_270to0:
        break;
    default:
         reg = PhMonFa_invalid;
         break;
    }

    return (PhMonFa) reg;
}

void evgPhaseMonSel::setMonReset(bool isReset) {
    epicsUInt32 reg;

    reg = nat_ioread32(m_pPhReg);

    reg |= EVG_FPInPhMon_PHCLR_mask;

    nat_iowrite32(m_pPhReg, reg);
}

bool evgPhaseMonSel::getMonReset() const {
    bool isReset;
    epicsUInt32 reg;

    reg = nat_ioread32(m_pPhReg);

    isReset = 
      ((reg & EVG_FPInPhMon_PHRE_mask) == (PhMonRi_reset << EVG_FPInPhMon_PHRE_shift)) &&
      ((reg & EVG_FPInPhMon_PHFE_mask) == (PhMonFa_reset << EVG_FPInPhMon_PHFE_shift));

    return isReset;
}

void evgPhaseMonSel::setPhaseSelRaw(epicsUInt16 phSel) {
    setPhaseSel( (PhSel) phSel);
}

epicsUInt16 evgPhaseMonSel::getPhaseSelRaw() const {
    return (epicsUInt16) getPhaseSel();
}

void evgPhaseMonSel::setPhaseSel(PhSel phSel) {
    epicsUInt32 reg;

    switch (phSel) {
        case PhSel_0Deg: 
        case PhSel_90Deg:
        case PhSel_180Deg:
        case PhSel_270Deg:
            break;
        default:
            throw std::runtime_error("Front input phase select invalid");
            break;
    }

    reg = nat_ioread32(m_pPhReg);

    reg &= ~EVG_FPInPhMon_PHSEL_mask;
    reg |= phSel << EVG_FPInPhMon_PHSEL_shift;

    nat_iowrite32(m_pPhReg, reg);
}

evgPhaseMonSel::PhSel evgPhaseMonSel::getPhaseSel() const {
    epicsUInt32 reg;

    reg = nat_ioread32(m_pPhReg);

    reg &= EVG_FPInPhMon_PHSEL_mask;
    reg >>= EVG_FPInPhMon_PHSEL_shift;

    return (PhSel) reg;
}

bool evgPhaseMonSel::isDBusOnRiEdge() const {
    return (nat_ioread32(m_pPhReg) & EVG_FPInPhMon_DBPH_mask);
}

