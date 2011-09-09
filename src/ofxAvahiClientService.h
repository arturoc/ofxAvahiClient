/*
 * ofxAvahiClient.h
 *
 *  Created on: 09/09/2011
 *      Author: arturo
 */

#ifndef OFXAVAHICLIENT_H_
#define OFXAVAHICLIENT_H_

#include <avahi-client/client.h>
#include <avahi-common/simple-watch.h>
#include <avahi-client/publish.h>
#include <avahi-client/lookup.h>

#include "ofConstants.h"
#include "ofxThread.h"
#include "ofEvents.h"
#include "ofxAvahiService.h"

class ofxAvahiClientService: public ofxThread {
public:
	ofxAvahiClientService();
	virtual ~ofxAvahiClientService();

	// service types: http://www.dns-sd.org/ServiceTypes.html
	// service type should be _type._protocol
	// ie: an http server should be _http._tcp
	// osc server: _oscit._udp
	bool start(string service_name, string type, int port);
	void close();

	static string LOG_NAME;

protected:
	void threadedFunction();

private:
	static void client_cb(AvahiClient *s, AvahiClientState state, ofxAvahiClientService *client);
	static void modify_cb(AVAHI_GCC_UNUSED AvahiTimeout *e, ofxAvahiClientService *client);
	static void entry_group_cb(AvahiEntryGroup *g, AvahiEntryGroupState state, AVAHI_GCC_UNUSED ofxAvahiClientService *userdata);

	static void service_browser_cb (AvahiServiceBrowser *b, AvahiIfIndex interface, AvahiProtocol protocol, AvahiBrowserEvent event, const char *name, const char *type, const char *domain, AvahiLookupResultFlags flags, ofxAvahiClientService *userdata);
	static void service_resolver_cb(AvahiServiceResolver *r, AvahiIfIndex interface, AvahiProtocol protocol, AvahiResolverEvent event, const char *name, const char *type, const char *domain, const char *host_name, const AvahiAddress *a, uint16_t port, AvahiStringList *txt, AvahiLookupResultFlags flags, ofxAvahiClientService *userdata);
	void create_services(AvahiClient *c);
	AvahiSimplePoll * poll;
	AvahiClient * client;
	AvahiEntryGroup *group;
	string name;
	string type;
	int port;
};

#endif /* OFXAVAHICLIENT_H_ */
