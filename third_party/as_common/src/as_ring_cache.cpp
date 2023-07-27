
/******************************************************************************
   ��Ȩ���� (C), 2008-2011, M.Kernel

 ******************************************************************************
  �ļ���          : as_ring_cache.cpp
  �汾��          : 1.0
  ����            : hexin
  ��������        : 2008-08-07
  ����޸�        :
  ��������        : ���λ�����
  �����б�        :
  �޸���ʷ        :
  1 ����          :
    ����          :
    �޸�����      :
*******************************************************************************/


#include "as_ring_cache.h"
extern "C"{
#include "as_config.h"
#include "as_basetype.h"
#include "as_common.h"
#include "as_mutex.h"
#include "as_event.h"
#include "as_thread.h"
#include "as_time.h"
#include "as_json.h"
}

as_ring_cache::as_ring_cache()
{
    m_pBuffer = NULL;
    m_ulBufferSize = 0;
    m_ulReader = 0;
    m_ulWriter = 0;
    m_ulDataSize = 0;

    m_pMutex = as_create_mutex();
}

as_ring_cache::~as_ring_cache()
{
    as_mutex_lock(m_pMutex);

    if(NULL != m_pBuffer) {
        try 
        {
            delete[] m_pBuffer;
            m_pBuffer = NULL;
        }
        catch(...)
        {
            m_pBuffer = NULL;
        }
        m_pBuffer = NULL;
    }
    m_ulBufferSize = 0;
    m_ulReader = 0;
    m_ulWriter = 0;
    m_ulDataSize = 0;
    as_mutex_unlock(m_pMutex);
    as_destroy_mutex(m_pMutex);
}

//设置缓冲区大小，返回设置完成后缓冲的大小
uint32_t as_ring_cache::SetCacheSize(uint32_t ulCacheSize)
{
    as_mutex_lock(m_pMutex);
    //清空数据
    m_ulReader = 0;
    m_ulWriter = 0;
    m_ulDataSize = 0;

    //缓冲区大小未发生变化，不需要重新申请内存
    if(ulCacheSize == m_ulBufferSize)
    {
        as_mutex_unlock(m_pMutex);
        return m_ulBufferSize;
    }

    //缓冲区大小发生变化，需要重新申请内存
    //释放当前缓冲内存
    if(NULL != m_pBuffer)
    {
        try
        {
            delete[] m_pBuffer;
            m_pBuffer = NULL;
        }
        catch(...)
        {
            m_pBuffer = NULL;
        }
        m_pBuffer = NULL;
    }

    //申请新缓冲内存
    m_ulBufferSize = ulCacheSize;
    if(m_ulBufferSize > 0)
    {
        try
        {
            m_pBuffer = new char[m_ulBufferSize];
        }
        catch(...)
        {
        }

        if(NULL == m_pBuffer)
        {//申请失败
            m_ulBufferSize = 0;
        }
    }

    as_mutex_unlock(m_pMutex);
    return m_ulBufferSize;
}

//获得当前缓冲区大小
uint32_t as_ring_cache::GetCacheSize() const
{
    return m_ulBufferSize;
}

//查看指定长度数据，但缓冲中仍然保存这些数据，返回实际读取数据长度
uint32_t as_ring_cache::Peek(char* pBuf, uint32_t ulPeekLen)/*lint -e1714*/
{
    uint32_t ulResult = 0;

    as_mutex_lock(m_pMutex);

    //计算实际可读取的长度
    ulResult = m_ulDataSize>ulPeekLen?ulPeekLen:m_ulDataSize;
    if(0 == ulResult)
    {
        as_mutex_unlock(m_pMutex);
        return ulResult;
    }

    //数据呈单段分布
    if(m_ulReader < m_ulWriter)/*lint -e613*/
    {//ooo********ooooo
        ::memcpy(pBuf, m_pBuffer+m_ulReader, ulResult);/*lint -e670*/
        as_mutex_unlock(m_pMutex);
        return ulResult;
    }

    //数据呈两段分布，m_ulReader等于m_ulWriter时数据满，也是两段
    //*B*oooooooo**A**
    uint32_t ulASectionLen = m_ulBufferSize - m_ulReader;//A段数据长度
    if(ulResult <= ulASectionLen)//A段数据长度足够
    {
        ::memcpy(pBuf, m_pBuffer+m_ulReader, ulResult);
    }
    else//A段数据长度不够，还需要从B段读取
    {
        //先读A段，再从B段补读
        ::memcpy(pBuf, m_pBuffer+m_ulReader, ulASectionLen);
        ::memcpy(pBuf+ulASectionLen, m_pBuffer, ulResult-ulASectionLen);
    }

    as_mutex_unlock(m_pMutex);
    return ulResult;
}

//读取指定长度数据，返回实际读取数据长度
uint32_t as_ring_cache::Read(char* pBuf, uint32_t ulReadLen)
{
    uint32_t ulResult = 0;

    as_mutex_lock(m_pMutex);

    //计算实际可读取的长度
    ulResult = m_ulDataSize>ulReadLen?ulReadLen:m_ulDataSize;
    if(0 == ulResult)
    {
        as_mutex_unlock(m_pMutex);
        return ulResult;
    }

    //数据呈单段分布
    if(m_ulReader < m_ulWriter)
    {//ooo********ooooo
        ::memcpy(pBuf, m_pBuffer+m_ulReader, ulResult);

        //数据被读取，更新读取位置
        m_ulReader += ulResult;/*lint -e414*/
        m_ulReader %= m_ulBufferSize;
        //数据已被读取，更新缓冲区数据长度
        m_ulDataSize -= ulResult;

        as_mutex_unlock(m_pMutex);
        return ulResult;
    }

    //数据呈两段分布，m_ulReader等于m_ulWriter时数据满，也是两段
    //*B*oooooooo**A**
    uint32_t ulASectionLen = m_ulBufferSize - m_ulReader;//A段数据长度
    if(ulResult <= ulASectionLen)//A段数据长度够
    {
        //PCLINTע��˵������Ҫ���ǵ���Ч�ʣ����������飬ָ��ʹ��������
        ::memcpy(pBuf, m_pBuffer+m_ulReader, ulResult);/*lint -e826*/

        //数据被读取，更新读取位置
        m_ulReader += ulResult;
        m_ulReader %= m_ulBufferSize;
    }
    else//A段数据长度不够，还需要从B段读取
    {
        //先读A段，再从B段补读
        ::memcpy(pBuf, m_pBuffer+m_ulReader, ulASectionLen);
        //PCLINTע��˵������Ҫ���ǵ���Ч�ʣ����������飬��ȷ������Ч��
        ::memcpy(pBuf+ulASectionLen, m_pBuffer, ulResult-ulASectionLen);/*lint -e429*/
        m_ulReader = ulResult - ulASectionLen;//���ݱ���ȡ�����¶�ȡλ��
    }
    //数据已被读取，更新缓冲区数据长度
    m_ulDataSize -= ulResult;

    as_mutex_unlock(m_pMutex);
    return ulResult;
}

//写指定长度数据，返回实际写数据长度，若缓冲区空间不够，禁止写入
uint32_t as_ring_cache::Write(const char* pBuf, uint32_t ulWriteLen)
{
    uint32_t ulResult = 0;

    as_mutex_lock(m_pMutex);

    //计算实际可写入长度，若空余缓冲区不够，则不写入数据
    ulResult = (m_ulBufferSize-m_ulDataSize)<ulWriteLen?0:ulWriteLen;
    if(0 == ulResult)
    {
        as_mutex_unlock(m_pMutex);
        return ulResult;
    }

    //空余空间呈单段分布
    if(m_ulReader > m_ulWriter)
    {//***oooooooo*****
        ::memcpy(m_pBuffer+m_ulWriter, pBuf, ulResult);

        //数据已写入，更新写入位置
        m_ulWriter += ulResult;
        m_ulWriter %= m_ulBufferSize;
        //数据已写入，更新缓冲区数据长度
        m_ulDataSize += ulResult;

        as_mutex_unlock(m_pMutex);
        return ulResult;
    }

    //空余空间呈两段分布，m_ulReader等于m_ulWriter时无数据，也是两段分布
    //oBo********ooAoo
    uint32_t ulASectionLen = m_ulBufferSize - m_ulWriter;//A段空余缓冲长度
    if(ulResult <= ulASectionLen)//A段空余缓冲长度足够
    {/*lint -e669*/
        //PCLINTע��˵������Ҫ���ǵ���Ч�ʣ�����������
        ::memcpy(m_pBuffer+m_ulWriter, pBuf, ulResult);

        //数据已写入，更新写入位置
        m_ulWriter += ulResult;
        m_ulWriter %= m_ulBufferSize;
    }
    else//A段空余缓冲长度不够，还要向B段写入
    {
        ::memcpy(m_pBuffer+m_ulWriter, pBuf, ulASectionLen);
        //PCLINTע��˵������Ҫ���ǵ���Ч�ʣ�����������
        ::memcpy(m_pBuffer, pBuf+ulASectionLen, ulResult-ulASectionLen);/*lint !e662*/
        m_ulWriter = ulResult - ulASectionLen;//数据已写入，更新写入位置
    }

    //数据已写入，更新缓冲区数据长度
    m_ulDataSize += ulResult;

    as_mutex_unlock(m_pMutex);
    return ulResult;
}

//获得当前缓冲中数据大小
uint32_t as_ring_cache::GetDataSize() const
{
    return m_ulDataSize;
}

//获得当前空余缓冲大小
uint32_t as_ring_cache::GetEmptySize() const
{
    return (m_ulBufferSize - m_ulDataSize);
}

//清空数据
void as_ring_cache::Clear()
{
    as_mutex_lock(m_pMutex);
    m_ulReader = 0;
    m_ulWriter = 0;
    m_ulDataSize = 0;
    as_mutex_unlock(m_pMutex);
}

//获得当前缓冲区中数据长度和缓冲区长度的比例的百分数
uint32_t as_ring_cache::GetUsingPercent() const
{
    if (0 == m_ulBufferSize) {
        return 0;
    }

    uint32_t ulCurrentUsingPercent = (m_ulDataSize*100)/m_ulBufferSize;
    return ulCurrentUsingPercent;
}

