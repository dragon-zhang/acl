#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/mqtt/mqtt_pingresp.hpp"
#endif

namespace acl {

mqtt_pingresp::mqtt_pingresp()
: mqtt_message(MQTT_PINGRESP)
{
}

mqtt_pingresp::~mqtt_pingresp(void) {}

bool mqtt_pingresp::to_string(string& out) {
	bool old_mode = out.get_bin();
	out.set_bin(true);

	this->set_data_length(0);

	if (!this->pack_header(out)) {
		out.set_bin(old_mode);
		return false;
	}

	out.set_bin(old_mode);
	return true;
}

} // namespace acl
