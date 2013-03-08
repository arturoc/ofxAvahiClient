/*
 * ofxAvahiClientResolver.h
 *
 *  Created on: 09/09/2011
 *      Author: arturo
 */

#pragma once

#include <avahi-client/client.h>
#include <avahi-common/simple-watch.h>
#include <avahi-client/publish.h>
#include <avahi-client/lookup.h>

#include "ofConstants.h"
#include "ofThread.h"
#include "ofxAvahiService.h"
#include "ofEvents.h"

class ofxAvahiClientBrowser : public ofThread {
public:
	ofxAvahiClientBrowser();
	virtual ~ofxAvahiClientBrowser();

	bool lookup(const string& type);
	void close();

	static string LOG_NAME;

	ofEvent<ofxAvahiService> serviceNewE;
	ofEvent<ofxAvahiService> serviceRemoveE;

protected:
	void threadedFunction();

private:
	AvahiClient * client;
	AvahiSimplePoll * poll;

	static void service_browser_cb (AvahiServiceBrowser *b, AvahiIfIndex interface, AvahiProtocol protocol, AvahiBrowserEvent event, const char *name, const char *type, const char *domain, AvahiLookupResultFlags flags, ofxAvahiClientBrowser *browser);
	static void service_resolver_cb(AvahiServiceResolver *r, AvahiIfIndex interface, AvahiProtocol protocol, AvahiResolverEvent event, const char *name, const char *type, const char *domain, const char *host_name, const AvahiAddress *a, uint16_t port, AvahiStringList *txt, AvahiLookupResultFlags flags, ofxAvahiClientBrowser *browser);
	static void client_cb(AvahiClient *s, AvahiClientState state, ofxAvahiClientBrowser *browser);
};
