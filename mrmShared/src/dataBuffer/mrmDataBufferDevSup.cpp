#include "mrmDataBufferDevSup.h"
#include "mrmDataBuffer.h"

mrmDataBufferDevSup::mrmDataBufferDevSup(const std::string &name, mrmDataBuffer *dataBuffer)
    :mrf::ObjectInst<mrmDataBufferDevSup>(name)
    ,m_data_buffer(dataBuffer)
{

}

bool mrmDataBufferDevSup::supportsRx() const
{
    if (m_data_buffer != NULL) return m_data_buffer->supportsRx();
    return false;
}

bool mrmDataBufferDevSup::supportsTx() const
{
    if (m_data_buffer != NULL) return m_data_buffer->supportsTx();
    return false;
}

bool mrmDataBufferDevSup::enabledRx() const
{
    if (m_data_buffer != NULL) return m_data_buffer->enabledRx();
    return false;
}

bool mrmDataBufferDevSup::enabledTx() const
{
    if (m_data_buffer != NULL) return m_data_buffer->enabledTx();
    return false;
}

void mrmDataBufferDevSup::enableRx(bool enable)
{
    if (m_data_buffer != NULL) m_data_buffer->enableRx(enable);
}

void mrmDataBufferDevSup::enableTx(bool enable)
{
    if (m_data_buffer != NULL) m_data_buffer->enableTx(enable);
}


OBJECT_BEGIN(mrmDataBufferDevSup) {

    OBJECT_PROP1("SupportsRx",&mrmDataBufferDevSup::supportsRx);
    OBJECT_PROP1("SupportsTx",&mrmDataBufferDevSup::supportsTx);

    OBJECT_PROP2("EnableRx", &mrmDataBufferDevSup::enabledRx, &mrmDataBufferDevSup::enableRx);
    OBJECT_PROP2("EnableTx", &mrmDataBufferDevSup::enabledTx, &mrmDataBufferDevSup::enableTx);

} OBJECT_END(mrmDataBufferDevSup)
