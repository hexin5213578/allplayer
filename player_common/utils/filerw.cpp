//FileRW的实现文件

#include "filerw.h"

const char* g_key = "0123456789abcdef0123456789abcdef";
const char* g_iv = "0123456789abcdef";
uint32_t g_idr_mark = 0xAAAAAAAA;
uint16_t g_payload_marker = 0xA1A2;
uint16_t g_frame_prefix = 0xB1B2;

namespace avs_toolkit
{
    
FileRW::~FileRW() {
    close();
}

bool FileRW::open(const std::string& file_name, bool write, bool is_binary) 
{
    close();
    m_write = write;
	if (m_write) {
        m_file.open(file_name, is_binary ? std::ios::binary | std::ios::out : std::ios::out);
	}
	else {
        m_file.open(file_name, is_binary ? std::ios::binary | std::ios::ate | std::ios::in : std::ios::in | std::ios::ate);
        m_file_size = m_file.tellg();
		m_file.seekg(0, m_file.beg);
	}
    
    return m_file.is_open() && m_file.good();
}

size_t FileRW::size() 
{
    if (!m_file.is_open()) {
        return 0;
    }

    if (m_write) {
        return (m_file << std::flush).tellp();
    }
    else {
        return m_file_size;
    }
}

bool FileRW::read(size_t pos, std::ios_base::seekdir seek, char* data, size_t size)
{
    if (!m_file.is_open()) {
        return false;
    }
    
    if (!(!pos && (seek == std::ios::cur))) {
        if (!m_file.seekg(pos, seek).good())
            return false;
    }

    return m_file.read(data, size).good();
}

void FileRW::close() 
{
    if (m_file.is_open()) {
        m_file.close();
    }
}

} //namespace crypto