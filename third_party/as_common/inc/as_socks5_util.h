#pragma once

#include <stdint.h>

// Defaults
enum class SOCKS5_DEFAULTS : uint8_t {
	RSV = 0x00,
	SUPPORT_AUTH = 0x01,
	VERSION = 0x05,
	VER_USERPASS = 0x01
};

// Currently supported AUTH Types (0x00 is default)
enum class SOCKS5_AUTH_TYPES : uint8_t  {
	NOAUTH = 0x00,
	USERPASS = 0x02,
};

// DNS resolve locally & make resolution on SOCKS5's endpoint
enum class SOCKS5_RESOLVE {
	REMOTE_RESOLVE = 0x01,
	LOCAL_RESOLVE = 0x02
};

// Anonymous SOCKS5 connect (NOAUTH default)
enum class SOCKS5_CGREETING_NOAUTH : uint8_t {
	VERSION = static_cast<uint8_t>(SOCKS5_DEFAULTS::VERSION),
	NAUTH = static_cast<uint8_t>(SOCKS5_DEFAULTS::SUPPORT_AUTH),
	AUTH = static_cast<uint8_t>(SOCKS5_AUTH_TYPES::NOAUTH)
};

// User/Pass SOCKS5 AUTH
enum class SOCKS5_CGREETING_AUTH : uint8_t {
	VERSION = static_cast<uint8_t>(SOCKS5_DEFAULTS::VERSION),
	NAUTH = static_cast<uint8_t>(SOCKS5_DEFAULTS::SUPPORT_AUTH),
	AUTH = static_cast<uint8_t>(SOCKS5_AUTH_TYPES::USERPASS)
};

enum class SOCKS5_ADDR_TYPE : uint8_t {
	IPv4 = 0x01,
	SOCKS_DOMAIN = 0x03,
	IPv6 = 0x04
};

// SOCKS5 Client connection request commands
enum class SOCKS5_CCONNECTION_CMD : uint8_t {
	TCP_IP_STREAM = 0x01,
	TCP_IP_PORT_BIND = 0x02,
	UDP_PORT = 0x03
};