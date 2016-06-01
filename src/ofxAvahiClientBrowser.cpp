/*
 * ofxAvahiClientResolver.cpp
 *
 *  Created on: 09/09/2011
 *      Author: arturo
 */

#include "ofxAvahiClientBrowser.h"

#include <avahi-common/malloc.h>
#include <avahi-common/error.h>
#include <avahi-common/timeval.h>
#include <avahi-common/alternative.h>

#include "ofLog.h"
#include "ofUtils.h"

string ofxAvahiClientBrowser::LOG_NAME = "ofxAvahiClientBrowser";
ofMutex ofxAvahiClientBrowser::mutex;


ofxAvahiClientBrowser::ofxAvahiClientBrowser():
client(NULL),
poll(NULL)
{

}

ofxAvahiClientBrowser::~ofxAvahiClientBrowser() {
	close();
}

void ofxAvahiClientBrowser::client_cb(AvahiClient *s, AvahiClientState state, ofxAvahiClientBrowser *browser){
	//assert(browser);

	/* Called whenever the client or server state changes */

	switch (state) {
		case AVAHI_CLIENT_S_RUNNING:

			/* The server has startup successfully and registered its host
			 * name on the network, so it's time to create our services */
			//delay it till function returns, we don't have the client reference here yet
			break;

		case AVAHI_CLIENT_FAILURE:

			ofLogError(LOG_NAME) << "Client failure:" << avahi_strerror(avahi_client_errno(browser->client));
      //ofxAvahiClientBrowser::mutex.lock();
			avahi_simple_poll_quit(browser->poll);
      //ofxAvahiClientBrowser::mutex.unlock();

			break;

		case AVAHI_CLIENT_S_COLLISION:

			/* Let's drop our registered services. When the server is back
			 * in AVAHI_SERVER_RUNNING state we will register them
			 * again with the new host name. */

		case AVAHI_CLIENT_S_REGISTERING:

			/* The server records are now being established. This
			 * might be caused by a host name change. We need to wait
			 * for our own records to register until the host name is
			 * properly esatblished. */


			break;

		case AVAHI_CLIENT_CONNECTING:
			break;
	}
}

bool ofxAvahiClientBrowser::lookup(const string& type){
	int err;
	//struct timeval tv;

  ofxAvahiClientBrowser::mutex.lock();
  poll = avahi_simple_poll_new();
  ofxAvahiClientBrowser::mutex.unlock();
	if (!(poll)) {
		ofLogError(LOG_NAME) << "Failed to create simple poll object";
		return false;
	}

  ofxAvahiClientBrowser::mutex.lock();
	client = avahi_client_new(avahi_simple_poll_get(poll),(AvahiClientFlags)0,(AvahiClientCallback)&client_cb,this,&err);
  ofxAvahiClientBrowser::mutex.unlock();

	if(!client){
		ofLogError(LOG_NAME) << "Failed to create avahi client" << avahi_strerror(err);
		close();
		return false;
	}

	avahi_service_browser_new(client,AVAHI_IF_UNSPEC,AVAHI_PROTO_INET,type.c_str(),NULL,(AvahiLookupFlags)0,(AvahiServiceBrowserCallback)service_browser_cb,this);

	startThread(true);

	return true;
}

void ofxAvahiClientBrowser::close(){
    if (client) {
        ofxAvahiClientBrowser::mutex.lock();
        avahi_client_free(client);
        ofxAvahiClientBrowser::mutex.unlock();
    }

    if (poll) {
        ofxAvahiClientBrowser::mutex.lock();
        avahi_simple_poll_free(poll);
        ofxAvahiClientBrowser::mutex.unlock();
    }
}

void ofxAvahiClientBrowser::threadedFunction(){
  ofxAvahiClientBrowser::mutex.lock();
	avahi_simple_poll_loop(poll);
  ofxAvahiClientBrowser::mutex.unlock();
}

void ofxAvahiClientBrowser::service_resolver_cb(AvahiServiceResolver *r, AvahiIfIndex interface, AvahiProtocol protocol, AvahiResolverEvent event, const char *name, const char *type, const char *domain, const char *host_name, const AvahiAddress *a, uint16_t port, AvahiStringList *txt, AvahiLookupResultFlags flags, ofxAvahiClientBrowser *browser){
	if(event == AVAHI_RESOLVER_FOUND){
		int a4 = a->data.ipv4.address >> 24;
		int a3 = (a->data.ipv4.address << 8) >> 24;
		int a2 = (a->data.ipv4.address << 16) >> 24;
		int a1 = (a->data.ipv4.address << 24) >> 24;
		string ip = ofToString(a1)+"."+ofToString(a2)+"."+ofToString(a3)+"."+ofToString(a4);
		ofLogVerbose(LOG_NAME) << "service_resolver: " << event << name << type << domain << host_name << ip << port;
		ofxAvahiService service;
		service.domain = domain;
		service.host_name = host_name;
		service.ip = ip;
		service.port = port;
		service.name = name;
		ofNotifyEvent(browser->serviceNewE,service);
	}

}

void ofxAvahiClientBrowser::service_browser_cb (AvahiServiceBrowser *b, AvahiIfIndex interface, AvahiProtocol protocol, AvahiBrowserEvent event, const char *name, const char *type, const char *domain, AvahiLookupResultFlags flags, ofxAvahiClientBrowser *browser){
	ofLogVerbose(LOG_NAME) << "service_browser: " << event << name << type << domain;
	if(event==AVAHI_BROWSER_NEW){
		avahi_service_resolver_new(browser->client,interface,protocol,name,type,domain,AVAHI_PROTO_INET,(AvahiLookupFlags)0,(AvahiServiceResolverCallback)service_resolver_cb,browser);
	}else if(event==AVAHI_BROWSER_REMOVE){
		ofxAvahiService service;
		service.domain = domain;
		service.name = name;
		ofNotifyEvent(browser->serviceRemoveE,service);
	}
}
