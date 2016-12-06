#include "evrSequencer.h"

evrSequencer::evrSequencer()
{
    m_isSynced = true;
    scanIoInit(&m_irq_sos);
    scanIoInit(&m_irq_eos);
    scanIoInit(&m_irq_synced);
}
