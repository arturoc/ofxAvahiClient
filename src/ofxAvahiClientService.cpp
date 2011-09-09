/*
 * ofxAvahiClient.cpp
 *
 *  Created on: 09/09/2011
 *      Author: arturo
 */

#include "ofxAvahiClientService.h"
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>
#include <avahi-common/timeval.h>
#include <avahi-common/alternative.h>

#include "ofLog.h"
#include "ofUtils.h"

string ofxAvahiClientService::LOG_NAME = "ofxAvahiClient";

void ofxAvahiClientService::client_cb(AvahiClient *s, AvahiClientState state, ofxAvahiClientService *client){
	assert(client);

	/* Called whenever the client or server state changes */

	switch (state) {
		case AVAHI_CLIENT_S_RUNNING:

			/* The server has startup successfully and registered its host
			 * name on the network, so it's time to create our services */
			//delay it till function returns, we don't have the client reference here yet
			break;

		case AVAHI_CLIENT_FAILURE:

			ofLogError(LOG_NAME) << "Client failure:" << avahi_strerror(avahi_client_errno(client->client));
			avahi_simple_poll_quit(client->poll);

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

			if (client->group)
				avahi_entry_group_reset(client->group);

			break;

		case AVAHI_CLIENT_CONNECTING:
			break;
	}
}

void ofxAvahiClientService::modify_cb(AVAHI_GCC_UNUSED AvahiTimeout *e, ofxAvahiClientService *client) {
    AvahiClient *avahi_client = client->client;
    AvahiEntryGroup * group = client->group;

    /* If the server is currently running, we need to remove our
     * service and create it anew */
    if (avahi_client_get_state(avahi_client) == AVAHI_CLIENT_S_RUNNING) {

        /* Remove the old services */
        if (group)
            avahi_entry_group_reset(group);

        /* And create them again with the new name */
        client->create_services(avahi_client);
    }
}

void ofxAvahiClientService::entry_group_cb(AvahiEntryGroup *g, AvahiEntryGroupState state, AVAHI_GCC_UNUSED ofxAvahiClientService *client) {
    assert(g == client->group || client->group == NULL);
    client->group = g;

    /* Called whenever the entry group state changes */

    switch (state) {
        case AVAHI_ENTRY_GROUP_ESTABLISHED :
            /* The entry group has been established successfully */
            ofLogNotice(LOG_NAME) << "Entry group for service" << client->name << "successfully established";
            break;

        case AVAHI_ENTRY_GROUP_COLLISION : {
            char *n;

            /* A service name collision with a remote service
             * happened. Let's pick a new name */
            n = avahi_alternative_service_name(client->name.c_str());
            client->name = n;

            ofLogError(LOG_NAME) << "Service name collision, renaming service to" << client->name;

            /* And recreate the services */
            client->create_services(avahi_entry_group_get_client(g));
            break;
        }

        case AVAHI_ENTRY_GROUP_FAILURE :

            ofLogError(LOG_NAME) << "Entry group failure:" << avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g)));

            /* Some kind of failure happened while we were registering our services */
            avahi_simple_poll_quit(client->poll);
            break;

        case AVAHI_ENTRY_GROUP_UNCOMMITED:
        case AVAHI_ENTRY_GROUP_REGISTERING:
        	break;
    }
}

ofxAvahiClientService::ofxAvahiClientService():
poll(NULL),
client(NULL),
group(NULL)
{


}

ofxAvahiClientService::~ofxAvahiClientService() {
	close();
}

bool ofxAvahiClientService::start(string service_name, string _type, int _port){
	name = service_name;
	type = _type;
	port = _port;
	int err;
	//struct timeval tv;

	if (!(poll = avahi_simple_poll_new())) {
		ofLogError(LOG_NAME) << "Failed to create simple poll object";
		return false;
	}

	client = avahi_client_new(avahi_simple_poll_get(poll),(AvahiClientFlags)0,(AvahiClientCallback)&client_cb,this,&err);
	if(!client){
		ofLogError(LOG_NAME) << "Failed to create avahi client" << avahi_strerror(err);
		close();
	}

	create_services(client);

    /*avahi_simple_poll_get(poll)->timeout_new(
        avahi_simple_poll_get(poll),
        avahi_elapse_time(&tv, 1000*10, 0),
        (AvahiTimeoutCallback)&modify_cb,
        this);*/

	startThread(true,false);

    return true;
}


void ofxAvahiClientService::threadedFunction(){
	avahi_simple_poll_loop(poll);
}

void ofxAvahiClientService::close(){
    if(group)
    	avahi_entry_group_free(group);

    if (client)
        avahi_client_free(client);

    if (poll)
        avahi_simple_poll_free(poll);
}

void ofxAvahiClientService::create_services(AvahiClient *c) {
    int ret;
    assert(c);

    /* If this is the first time we're called, let's create a new
     * entry group if necessary */

    if (!group)
        if (!(group = avahi_entry_group_new(c, (AvahiEntryGroupCallback)&entry_group_cb, this))) {
            ofLogError(LOG_NAME) << "avahi_entry_group_new() failed:" << avahi_strerror(avahi_client_errno(c));
            goto fail;
        }

    /* If the group is empty (either because it was just created, or
     * because it was reset previously, add our entries.  */

    if (avahi_entry_group_is_empty(group)) {
        ofLogNotice(LOG_NAME) << "Adding service" << name;

        /* Create some random TXT data */
        //snprintf(r, sizeof(r), "random=%i", rand());

        /* We will now add two services and one subtype to the entry
         * group. The two services have the same name, but differ in
         * the service type (IPP vs. BSD LPR). Only services with the
         * same name should be put in the same entry group. */

        /* Add the service for IPP */
        if ((ret = avahi_entry_group_add_service(group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, (AvahiPublishFlags)0, name.c_str(), type.c_str(), NULL, NULL, port, NULL)) < 0) {

            if (ret == AVAHI_ERR_COLLISION)
                goto collision;

            ofLogError(LOG_NAME) << "Failed to add" << type << "service:" << avahi_strerror(ret);
            goto fail;
        }

        /* Add an additional (hypothetic) subtype */
        /*if ((ret = avahi_entry_group_add_service_subtype(group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, (AvahiPublishFlags)0, name.c_str(), "_printer._tcp", NULL, "_magic._sub._printer._tcp") < 0)) {
        	ofLogError(LOG_NAME) << "Failed to add subtype _magic._sub._printer._tcp:" << avahi_strerror(ret);
            goto fail;
        }*/

        /* Tell the server to register the service */
        if ((ret = avahi_entry_group_commit(group)) < 0) {
        	ofLogError(LOG_NAME) << "Failed to commit entry group:" << avahi_strerror(ret);
            goto fail;
        }
    }

    return;

collision:

    /* A service name collision with a local service happened. Let's
     * pick a new name */
    name = avahi_alternative_service_name(name.c_str());

    ofLogError(LOG_NAME) << "Service name collision, renaming service to" << name;

    avahi_entry_group_reset(group);

    create_services(c);
    return;

fail:
    avahi_simple_poll_quit(poll);
}
