#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/mqtt/mqtt_message.hpp"
#include "acl_cpp/mqtt/mqtt_connect.hpp"
#include "acl_cpp/mqtt/mqtt_connack.hpp"
#include "acl_cpp/mqtt/mqtt_publish.hpp"
#include "acl_cpp/mqtt/mqtt_puback.hpp"
#include "acl_cpp/mqtt/mqtt_pubrec.hpp"
#include "acl_cpp/mqtt/mqtt_pubrel.hpp"
#include "acl_cpp/mqtt/mqtt_pubcomp.hpp"
#include "acl_cpp/mqtt/mqtt_subscribe.hpp"
#include "acl_cpp/mqtt/mqtt_suback.hpp"
#include "acl_cpp/mqtt/mqtt_unsubscribe.hpp"
#include "acl_cpp/mqtt/mqtt_unsuback.hpp"
#include "acl_cpp/mqtt/mqtt_pingreq.hpp"
#include "acl_cpp/mqtt/mqtt_pingresp.hpp"
#include "acl_cpp/mqtt/mqtt_disconnect.hpp"
#include "acl_cpp/mqtt/mqtt_client.hpp"
#endif

namespace acl {

mqtt_client::mqtt_client(const char* addr, int conn_timeout, int rw_timeout)
: addr_(addr)
, conn_timeout_(conn_timeout)
, rw_timeout_(rw_timeout)
{
}

mqtt_client::~mqtt_client(void) {}

bool mqtt_client::open(void) {
	if (conn_.opened()) {
		return true;
	}

	if (!conn_.open(addr_, conn_timeout_, rw_timeout_)) {
		logger_error("connect redis %s error: %s",
			addr_.c_str(), last_serror());
		return false;
	}

	return true;
}

bool mqtt_client::send(mqtt_message& message) {
	if (!open()) {
		logger_error("connect server error: %s", last_serror());
		return false;
	}

	string buff;
	if (!message.to_string(buff)) {
		logger_error("build mqtt message error");
		return false;
	}
	if (buff.empty()) {
		logger_error("mqtt message empty");
		return false;
	}

	if (!open()) {
		logger_error("open error");
		return false;
	}

	if (conn_.write(buff) == -1) {
		logger_error("send message error=%s", last_serror());
		conn_.close();
		return false;
	}

	printf(">>>>send ok, buff size=%zd\r\n", buff.size());
	return true;
}

mqtt_message* mqtt_client::get_message(void) {
	mqtt_message header(MQTT_RESERVED_MIN);

	if (!read_header(header)) {
		conn_.close();
		logger_error("get header error");
		return NULL;
	}

	mqtt_message* body = create_body(header);
	if (body == NULL) {
		logger_error("create_message error");
		return NULL;
	}

	if (!read_body(header, *body)) {
		delete body;
		return NULL;
	}
	return body;
}

bool mqtt_client::read_header(mqtt_message& header) {
	char ch;
	if (!conn_.read(ch)) {
		logger_error("read header type error: %s", last_serror());
		return false;
	}
	printf(">>>read type=%d\n", ch);

	if (header.header_update(&ch, 1) != 0) {
		logger_error("invalid header type=%d", (int) ch);
		return false;
	}

	for (int i = 0; i < 4; i++) {
		if (!conn_.read(ch)) {
			logger_error("read one char error: %s, i=%d",
				last_serror(), i);
			return false;
		}
		if (header.header_update(&ch, 1) != 0) {
			logger_error("header_update error, ch=%d", (int) ch);
			return false;
		}
		if (header.header_finish()) {
			break;
		}
	}

	if (!header.header_finish()) {
		logger_error("get mqtt header error");
		return false;
	}

	return true;
}

bool mqtt_client::read_body(const mqtt_message& header, mqtt_message& body) {
	unsigned len = header.get_data_length();
	if (len == 0) {
		return true;
	}

	char buf[8192];
	while (len > 0) {
		size_t size = sizeof(buf) > len ? len : sizeof(buf);
		int n = conn_.read(buf, size);
		if (n == -1) {
			logger_error("read body error: %s", last_serror());
			return false;
		}

		len -= n;

		n = body.update(buf, (int) size);
		if (n == -1) {
			logger_error("update body error");
			return false;
		} else if (n != 0) {
			logger_error("invalid body data");
			return false;
		}
	}

	if (!body.is_finished()) {
		logger_error("body not finished!");
		return false;
	}
	return true;
}

mqtt_message* mqtt_client::create_body(const mqtt_message& header) {
	mqtt_type_t type = header.get_type();
	mqtt_message* message;
	unsigned dlen = header.get_data_length();

	switch (type) {
	case MQTT_CONNECT:
		message = NEW mqtt_connect();
		break;
	case MQTT_CONNACK:
		message = NEW mqtt_connack();
		break;
	case MQTT_PUBLISH:
		message = NEW mqtt_publish(dlen);
		break;
	case MQTT_PUBACK:
		message = NEW mqtt_puback();
		break;
	case MQTT_PUBREC:
		message = NEW mqtt_pubrec();
		break;
	case MQTT_PUBREL:
		message = NEW mqtt_pubrel();
		break;
	case MQTT_PUBCOMP:
		message = NEW mqtt_pubcomp();
		break;
	case MQTT_SUBSCRIBE:
		message = NEW mqtt_subscribe(dlen);
		break;
	case MQTT_SUBACK:
		message = NEW mqtt_suback(dlen);
		break;
	case MQTT_UNSUBSCRIBE:
		message = NEW mqtt_unsubscribe(dlen);
		break;
	case MQTT_UNSUBACK:
		message = NEW mqtt_unsuback();
		break;
	case MQTT_PINGREQ:
		message = NEW mqtt_pingreq();
		break;
	case MQTT_PINGRESP:
		message = NEW mqtt_pingresp();
		break;
	case MQTT_DISCONNECT:
		message = NEW mqtt_disconnect();
		break;
	default:
		logger_error("unknown mqtt type=%d", (int) type);
		message = NULL;
		break;
	}
	return message;
}

} // namespace acl
