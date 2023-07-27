#include "as_msg_cache.h"
#include "as_mem.h"

as_msg_block::~as_msg_block()
{
    AS_DELETE(m_pData, MULTI);
    m_ulSize = 0;
}

char* as_msg_block::base()
{
    return m_pData;
}

uint32_t as_msg_block::size()
{
	return m_ulSize;
}

uint32_t as_msg_block::length()
{
    if (m_pWrPtr >= m_pData)
    {
        uint32_t len = m_pWrPtr - m_pData;
        return len;
    }
    return 0;
}

int as_msg_block::write(const char* data, uint32_t len)
{
    if (m_ulSize < len) 
    {
        return AS_ERROR_CODE_MEM;
    }
    memcpy(m_pWrPtr, data, len);
    m_pWrPtr += len;
    return AS_ERROR_CODE_OK;
}

void as_msg_block::wr_ptr(int32_t len)
{
    m_pWrPtr = m_pData + len;
}

as_msg_block::as_msg_block(uint32_t size)
{
    m_ulSize = size;
    char* pData = AS_NEW(pData, size);
    m_pData = m_pWrPtr = pData;
}

as_msg_cache::as_msg_cache()
{
    m_blocksMutex = NULL;
    m_bigBlocksMutex = NULL;
    m_ulBlockSize = 0;
}

as_msg_cache::~as_msg_cache()
{
    release();
}

int32_t as_msg_cache::initBlocks(uint32_t blockSize)
{
    if (0 == blockSize)
    {
        return AS_ERROR_CODE_INVALID;
    }

    int32_t nRet = AS_ERROR_CODE_OK;
    as_msg_block* msgBlock = NULL;

    m_blocksMutex = as_create_mutex();
    if (NULL == m_blocksMutex)
    {
        return AS_ERROR_CODE_MEM;
    }

    as_mutex_lock(m_blocksMutex);
  
    for (uint32_t i = 0; i < blockSize; i++)
    {
        try
        {
            msgBlock = new as_msg_block(MSG_BLOCK_SIZE);
        }
        catch(...)
        {
            msgBlock = NULL;
            as_mutex_unlock(m_blocksMutex);
            return AS_ERROR_CODE_MEM;
        }
        m_msgBlocks.push_back(msgBlock);
    }
    as_mutex_unlock(m_blocksMutex);
    return AS_ERROR_CODE_OK;
}

void as_msg_cache::release()
{
    as_msg_block* block = nullptr;

    if (m_blocksMutex)
    {
        as_mutex_lock(m_blocksMutex);
        while (0 < m_msgBlocks.size())
        {
            block = m_msgBlocks.front();
            m_msgBlocks.pop_front();
            AS_DELETE(block);
        }
        as_mutex_unlock(m_blocksMutex);

        as_destroy_mutex(m_blocksMutex);
        m_blocksMutex = nullptr;
    }

    if (m_bigBlocksMutex)
    {
        as_mutex_lock(m_bigBlocksMutex);
        while (0 < m_bigMsgBlocks.size())
        {
            block = m_bigMsgBlocks.front();
            m_bigMsgBlocks.pop_front();
            AS_DELETE(block);
        }
        as_mutex_unlock(m_bigBlocksMutex);

        as_destroy_mutex(m_bigBlocksMutex);
        m_bigBlocksMutex = nullptr;
    }
}

as_msg_block* as_msg_cache::getMsgBlock(uint32_t buffSize)
{
    as_msg_block* msgBlock = nullptr;

    if (buffSize <= MSG_BLOCK_SIZE)
    {
        as_mutex_lock(m_blocksMutex);
        if (m_msgBlocks.empty())
        {
            as_mutex_unlock(m_blocksMutex);
            return nullptr;
        }

        msgBlock = m_msgBlocks.front();
        m_msgBlocks.pop_front();
        as_mutex_unlock(m_blocksMutex);
    }
    else
    {
        msgBlock = getBigMsgBlock();
    }
    return msgBlock;
}

void as_msg_cache::freeMsgBlock(as_msg_block* block)
{
    if (!block)
    {
        return;
    }

    block->wr_ptr(0);
    if (MSG_BLOCK_SIZE == block->size())
    {
        as_mutex_lock(m_blocksMutex);
        m_msgBlocks.push_back(block);
        as_mutex_unlock(m_blocksMutex);
    }
    else if (BIG_MSG_BLOCK_SIZE == block->size())
    {
        as_mutex_lock(m_bigBlocksMutex);
        m_bigMsgBlocks.push_back(block);
        as_mutex_unlock(m_bigBlocksMutex);
    }
    else
    {

    }
    return;
}

as_msg_block* as_msg_cache::getBigMsgBlock()
{
    if (NULL == m_bigBlocksMutex)
    {
        m_bigBlocksMutex = as_create_mutex();
        if (NULL == m_bigBlocksMutex)
        {
            return nullptr;
        }
    }

    as_msg_block* block = nullptr;

    as_mutex_lock(m_bigBlocksMutex);
    if (m_bigMsgBlocks.empty())
    {
        as_mutex_unlock(m_bigBlocksMutex);
        try
        {
            block = new as_msg_block(BIG_MSG_BLOCK_SIZE);
        }
        catch (...)
        {
            block = NULL;
            return block;
        }
    }
    else
    {
        block = m_bigMsgBlocks.front();
        m_bigMsgBlocks.pop_front();
        as_mutex_unlock(m_bigBlocksMutex);
    }
    return block;
}
