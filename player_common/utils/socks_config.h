#pragma once

typedef struct structSocksInfo
{
	std::string					 ip;
	uint16_t					 port;
	std::string					 username;
	std::string					 password;
}SocksInfo;

class SocksConfig
{
public:
	static SocksConfig* GetSocksConfigInstance() {
		static SocksConfig config;
		return &config;
	}

	void setSocksInfo(std::string& ip, uint16_t port, std::string& username, std::string& password) {
		m_inited = true;
		m_socks.ip = ip;
		m_socks.port = port;
		m_socks.username = username;
		m_socks.password = password;
	}

	void cancelSocks() {
		m_inited = false;
	}

	SocksInfo* getSocksInfo() {
		if (m_inited) {
			return &m_socks;
		}
		return nullptr;
	}
	
private:
	SocksConfig(){
		m_inited = false;
	}

	~SocksConfig() {

	}

private:
	SocksInfo					 m_socks;
	bool						 m_inited;
};

