#pragma once

#include <list>

#include "as_config.h"
#include "as_basetype.h"
#include "as_common.h"

#define MSG_BLOCK_SIZE              1500

#define BIG_MSG_BLOCK_SIZE          (16 * 1024)

class as_msg_block
{
public:
	friend class as_msg_cache;

public:
    virtual ~as_msg_block();
    char* base();
    uint32_t size();
    uint32_t length();

    int     write(const char* data, uint32_t len);
    void    wr_ptr(int32_t len);
  
protected:
    as_msg_block(uint32_t size);
   
private:
    char*       m_pData;
    char*       m_pWrPtr;
    uint32_t    m_ulSize;
};

typedef std::list<as_msg_block*>  MSG_BLOCK_LIST_T;

class as_msg_cache
{
public:
    as_msg_cache();
    virtual ~as_msg_cache();

    int32_t initBlocks(uint32_t blockSize);
    void    release();

    as_msg_block* getMsgBlock(uint32_t buffSize);
    void    freeMsgBlock(as_msg_block* msg);

private:
    as_msg_block* getBigMsgBlock();

private:
    MSG_BLOCK_LIST_T            m_msgBlocks;
    uint32_t                    m_ulBlockSize;
    as_mutex_t*                 m_blocksMutex;

    MSG_BLOCK_LIST_T            m_bigMsgBlocks;
    as_mutex_t*                 m_bigBlocksMutex;
};