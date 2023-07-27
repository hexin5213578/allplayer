//C++11文件读写类，可以指定读写的数据类型，支持二进制和文本文件

#ifndef _FILERW_H_
#define _FILERW_H_

#include <fstream>
#include <string>
#include <vector>

namespace avs_toolkit {

class FileRW 
{
public:
	FileRW() = default;
	~FileRW();

	bool open(const std::string& m_filename, bool write, bool is_binary = true);
	void close();
	
	bool eof() { return m_file.eof(); }
	
	bool seek(size_t pos, std::ios_base::seekdir seekd) {
		if (m_write) {
			return m_file.seekp(pos, seekd).good();
		}
		else {
			return m_file.seekg(pos, seekd).good();
		}
	}

	uint64_t getCur() {
		if (m_write) {
			return (m_file << std::flush).tellp();
		}
		else {
			return m_file.tellg();
		}
	}

	//获取文件大小
	size_t size();

	bool read(size_t pos, std::ios_base::seekdir seek, char* data, size_t size);
	
	//读取文件
	template<typename T>
	bool read(size_t pos, std::ios_base::seekdir seek, T& data);

	//写入文件
	template<typename T>
	bool write(const T* data, size_t size);

	//seek到文件位置后写入指定类型的数据
	template<typename T>
	bool writeAtPos(size_t pos, const T* data, size_t size);

	//0-start, 1-end
	bool seekAnchor(int end) {
		if (m_write) {
			m_file.seekp(0, end ? m_file.end : m_file.beg);
		}
		else {
			m_file.seekg(0, end ? m_file.end : m_file.beg);
		}
		return m_file.good();
	}

	void flush() {
		if (m_write) {
			m_file << std::flush;
		}
	}

private:
	bool			m_write;
	std::fstream	m_file;
	size_t			m_file_size;
};

template<typename T>
bool FileRW::read(size_t pos, std::ios_base::seekdir seek, T& data) {
	if (!m_file.is_open()) {
		return false;
	}
	if (!(!pos && (seek == std::ios::cur))) {
		if (!m_file.seekg(pos, seek).good()) {
			return false;
		}
	}
	return m_file.read((char*)&data, sizeof(T)).good();
}	

template<typename T>
bool FileRW::write(const T* data, size_t size) {
	if (!m_file.is_open()) {
		return false;
	}
	return m_file.write((char*)data, size).good();
}

template<typename T>
bool FileRW::writeAtPos(size_t pos, const T* data, size_t size) {
	if (!m_file.is_open()) {
		return false;
	}
    m_file.seekp(pos, std::ios::beg);
	return m_file.write((char*)&data[0], size).good();
}

} //namespace avs_toolkit

#endif //_FILERW_H_
