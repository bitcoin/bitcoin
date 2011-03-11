/* $Id: miniupnpcmodule.c,v 1.15 2010/06/09 10:23:01 nanard Exp $*/
/* Project : miniupnp
 * Author : Thomas BERNARD
 * website : http://miniupnp.tuxfamily.org/
 * copyright (c) 2007-2009 Thomas Bernard
 * This software is subjet to the conditions detailed in the
 * provided LICENCE file. */
#include <Python.h>
#define STATICLIB
#include "structmember.h"
#include "miniupnpc.h"
#include "upnpcommands.h"
#include "upnperrors.h"

/* for compatibility with Python < 2.4 */
#ifndef Py_RETURN_NONE
#define Py_RETURN_NONE return Py_INCREF(Py_None), Py_None
#endif

#ifndef Py_RETURN_TRUE
#define Py_RETURN_TRUE return Py_INCREF(Py_True), Py_True
#endif

#ifndef Py_RETURN_FALSE
#define Py_RETURN_FALSE return Py_INCREF(Py_False), Py_False
#endif

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
	struct UPNPDev * devlist;
	struct UPNPUrls urls;
	struct IGDdatas data;
	unsigned int discoverdelay;	/* value passed to upnpDiscover() */
	char lanaddr[16];	/* our ip address on the LAN */
	char * multicastif;
	char * minissdpdsocket;
} UPnPObject;

static PyMemberDef UPnP_members[] = {
	{"lanaddr", T_STRING_INPLACE, offsetof(UPnPObject, lanaddr),
	 READONLY, "ip address on the LAN"
	},
	{"discoverdelay", T_UINT, offsetof(UPnPObject, discoverdelay),
	 0/*READWRITE*/, "value in ms used to wait for SSDP responses"
	},
	/* T_STRING is allways readonly :( */
	{"multicastif", T_STRING, offsetof(UPnPObject, multicastif),
	 0, "IP of the network interface to be used for multicast operations"
	},
	{"minissdpdsocket", T_STRING, offsetof(UPnPObject, multicastif),
	 0, "path of the MiniSSDPd unix socket"
	},
	{NULL}
};

static void
UPnPObject_dealloc(UPnPObject *self)
{
	freeUPNPDevlist(self->devlist);
	FreeUPNPUrls(&self->urls);
	self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
UPnP_discover(UPnPObject *self)
{
	struct UPNPDev * dev;
	int i;
	PyObject *res = NULL;
	if(self->devlist)
	{
		freeUPNPDevlist(self->devlist);
		self->devlist = 0;
	} 
	Py_BEGIN_ALLOW_THREADS
	self->devlist = upnpDiscover((int)self->discoverdelay/*timeout in ms*/,
	                             0/* multicast if*/,
	                             0/*minissdpd socket*/,
								 0/*sameport flag*/);
	Py_END_ALLOW_THREADS
	/* Py_RETURN_NONE ??? */
	for(dev = self->devlist, i = 0; dev; dev = dev->pNext)
		i++;
	res = Py_BuildValue("i", i);
	return res;
}

static PyObject *
UPnP_selectigd(UPnPObject *self)
{
	int r;
Py_BEGIN_ALLOW_THREADS
	r = UPNP_GetValidIGD(self->devlist, &self->urls, &self->data,
	                     self->lanaddr, sizeof(self->lanaddr));
Py_END_ALLOW_THREADS
	if(r)
	{
		return Py_BuildValue("s", self->urls.controlURL);
	}
	else
	{
		/* TODO: have our own exception type ! */
		PyErr_SetString(PyExc_Exception, "No UPnP device discovered");
		return NULL;
	}
}

static PyObject *
UPnP_totalbytesent(UPnPObject *self)
{
	UNSIGNED_INTEGER i;
Py_BEGIN_ALLOW_THREADS
	i = UPNP_GetTotalBytesSent(self->urls.controlURL_CIF,
	                           self->data.CIF.servicetype);
Py_END_ALLOW_THREADS
	return Py_BuildValue("I", i);
}

static PyObject *
UPnP_totalbytereceived(UPnPObject *self)
{
	UNSIGNED_INTEGER i;
Py_BEGIN_ALLOW_THREADS
	i = UPNP_GetTotalBytesReceived(self->urls.controlURL_CIF,
		                           self->data.CIF.servicetype);
Py_END_ALLOW_THREADS
	return Py_BuildValue("I", i);
}

static PyObject *
UPnP_totalpacketsent(UPnPObject *self)
{
	UNSIGNED_INTEGER i;
Py_BEGIN_ALLOW_THREADS
	i = UPNP_GetTotalPacketsSent(self->urls.controlURL_CIF,
		                         self->data.CIF.servicetype);
Py_END_ALLOW_THREADS
	return Py_BuildValue("I", i);
}

static PyObject *
UPnP_totalpacketreceived(UPnPObject *self)
{
	UNSIGNED_INTEGER i;
Py_BEGIN_ALLOW_THREADS
	i = UPNP_GetTotalPacketsReceived(self->urls.controlURL_CIF,
		                          self->data.CIF.servicetype);
Py_END_ALLOW_THREADS
	return Py_BuildValue("I", i);
}

static PyObject *
UPnP_statusinfo(UPnPObject *self)
{
	char status[64];
	char lastconnerror[64];
	unsigned int uptime = 0;
	int r;
	status[0] = '\0';
	lastconnerror[0] = '\0';
Py_BEGIN_ALLOW_THREADS
	r = UPNP_GetStatusInfo(self->urls.controlURL, self->data.first.servicetype,
	                   status, &uptime, lastconnerror);
Py_END_ALLOW_THREADS
	if(r==UPNPCOMMAND_SUCCESS) {
		return Py_BuildValue("(s,I,s)", status, uptime, lastconnerror);
	} else {
		/* TODO: have our own exception type ! */
		PyErr_SetString(PyExc_Exception, strupnperror(r));
		return NULL;
	}
}

static PyObject *
UPnP_connectiontype(UPnPObject *self)
{
	char connectionType[64];
	int r;
	connectionType[0] = '\0';
Py_BEGIN_ALLOW_THREADS
	r = UPNP_GetConnectionTypeInfo(self->urls.controlURL,
	                               self->data.first.servicetype,
	                               connectionType);
Py_END_ALLOW_THREADS
	if(r==UPNPCOMMAND_SUCCESS) {
		return Py_BuildValue("s", connectionType);
	} else {
		/* TODO: have our own exception type ! */
		PyErr_SetString(PyExc_Exception, strupnperror(r));
		return NULL;
	}
}

static PyObject *
UPnP_externalipaddress(UPnPObject *self)
{
	char externalIPAddress[16];
	int r;
	externalIPAddress[0] = '\0';
Py_BEGIN_ALLOW_THREADS
	r = UPNP_GetExternalIPAddress(self->urls.controlURL,
	                              self->data.first.servicetype,
	                              externalIPAddress);
Py_END_ALLOW_THREADS
	if(r==UPNPCOMMAND_SUCCESS) {
		return Py_BuildValue("s", externalIPAddress);
	} else {
		/* TODO: have our own exception type ! */
		PyErr_SetString(PyExc_Exception, strupnperror(r));
		return NULL;
	}
}

/* AddPortMapping(externalPort, protocol, internalHost, internalPort, desc,
 *                remoteHost) 
 * protocol is 'UDP' or 'TCP' */
static PyObject *
UPnP_addportmapping(UPnPObject *self, PyObject *args)
{
	char extPort[6];
	unsigned short ePort;
	char inPort[6];
	unsigned short iPort;
	const char * proto;
	const char * host;
	const char * desc;
	const char * remoteHost;
	int r;
	if (!PyArg_ParseTuple(args, "HssHss", &ePort, &proto,
	                                     &host, &iPort, &desc, &remoteHost))
        return NULL;
Py_BEGIN_ALLOW_THREADS
	sprintf(extPort, "%hu", ePort);
	sprintf(inPort, "%hu", iPort);
	r = UPNP_AddPortMapping(self->urls.controlURL, self->data.first.servicetype,
	                        extPort, inPort, host, desc, proto, remoteHost);
Py_END_ALLOW_THREADS
	if(r==UPNPCOMMAND_SUCCESS)
	{
		Py_RETURN_TRUE;
	}
	else
	{
		// TODO: RAISE an Exception. See upnpcommands.h for errors codes.
		// upnperrors.c
		//Py_RETURN_FALSE;
		/* TODO: have our own exception type ! */
		PyErr_SetString(PyExc_Exception, strupnperror(r));
		return NULL;
	}
}

/* DeletePortMapping(extPort, proto, removeHost='')
 * proto = 'UDP', 'TCP' */
static PyObject *
UPnP_deleteportmapping(UPnPObject *self, PyObject *args)
{
	char extPort[6];
	unsigned short ePort;
	const char * proto;
	const char * remoteHost = "";
	int r;
	if(!PyArg_ParseTuple(args, "Hs|z", &ePort, &proto, &remoteHost))
		return NULL;
Py_BEGIN_ALLOW_THREADS
	sprintf(extPort, "%hu", ePort);
	r = UPNP_DeletePortMapping(self->urls.controlURL, self->data.first.servicetype,
	                           extPort, proto, remoteHost);
Py_END_ALLOW_THREADS
	if(r==UPNPCOMMAND_SUCCESS) {
		Py_RETURN_TRUE;
	} else {
		/* TODO: have our own exception type ! */
		PyErr_SetString(PyExc_Exception, strupnperror(r));
		return NULL;
	}
}

static PyObject *
UPnP_getportmappingnumberofentries(UPnPObject *self)
{
	unsigned int n = 0;
	int r;
Py_BEGIN_ALLOW_THREADS
	r = UPNP_GetPortMappingNumberOfEntries(self->urls.controlURL,
	                                   self->data.first.servicetype,
									   &n);
Py_END_ALLOW_THREADS
	if(r==UPNPCOMMAND_SUCCESS) {
		return Py_BuildValue("I", n);
	} else {
		/* TODO: have our own exception type ! */
		PyErr_SetString(PyExc_Exception, strupnperror(r));
		return NULL;
	}
}

/* GetSpecificPortMapping(ePort, proto) 
 * proto = 'UDP' or 'TCP' */
static PyObject *
UPnP_getspecificportmapping(UPnPObject *self, PyObject *args)
{
	char extPort[6];
	unsigned short ePort;
	const char * proto;
	char intClient[16];
	char intPort[6];
	unsigned short iPort;
	if(!PyArg_ParseTuple(args, "Hs", &ePort, &proto))
		return NULL;
Py_BEGIN_ALLOW_THREADS
	sprintf(extPort, "%hu", ePort);
	UPNP_GetSpecificPortMappingEntry(self->urls.controlURL,
	                                 self->data.first.servicetype,
									 extPort, proto,
									 intClient, intPort);
Py_END_ALLOW_THREADS
	if(intClient[0])
	{
		iPort = (unsigned short)atoi(intPort);
		return Py_BuildValue("(s,H)", intClient, iPort);
	}
	else
	{
		Py_RETURN_NONE;
	}
}

/* GetGenericPortMapping(index) */
static PyObject *
UPnP_getgenericportmapping(UPnPObject *self, PyObject *args)
{
	int i, r;
	char index[8];
	char intClient[16];
	char intPort[6];
	unsigned short iPort;
	char extPort[6];
	unsigned short ePort;
	char protocol[4];
	char desc[80];
	char enabled[6];
	char rHost[64];
	char duration[16];	/* lease duration */
	unsigned int dur;
	if(!PyArg_ParseTuple(args, "i", &i))
		return NULL;
Py_BEGIN_ALLOW_THREADS
	snprintf(index, sizeof(index), "%d", i);
	rHost[0] = '\0'; enabled[0] = '\0';
	duration[0] = '\0'; desc[0] = '\0';
	extPort[0] = '\0'; intPort[0] = '\0'; intClient[0] = '\0';
	r = UPNP_GetGenericPortMappingEntry(self->urls.controlURL,
	                                    self->data.first.servicetype,
										index,
										extPort, intClient, intPort,
										protocol, desc, enabled, rHost,
										duration);
Py_END_ALLOW_THREADS
	if(r==UPNPCOMMAND_SUCCESS)
	{
		ePort = (unsigned short)atoi(extPort);
		iPort = (unsigned short)atoi(intPort);
		dur = (unsigned int)strtoul(duration, 0, 0);
		return Py_BuildValue("(H,s,(s,H),s,s,s,I)",
		                     ePort, protocol, intClient, iPort,
		                     desc, enabled, rHost, dur);
	}
	else
	{
		Py_RETURN_NONE;
	}
}

/* miniupnpc.UPnP object Method Table */
static PyMethodDef UPnP_methods[] = {
    {"discover", (PyCFunction)UPnP_discover, METH_NOARGS,
     "discover UPnP IGD devices on the network"
    },
	{"selectigd", (PyCFunction)UPnP_selectigd, METH_NOARGS,
	 "select a valid UPnP IGD among discovered devices"
	},
	{"totalbytesent", (PyCFunction)UPnP_totalbytesent, METH_NOARGS,
	 "return the total number of bytes sent by UPnP IGD"
	},
	{"totalbytereceived", (PyCFunction)UPnP_totalbytereceived, METH_NOARGS,
	 "return the total number of bytes received by UPnP IGD"
	},
	{"totalpacketsent", (PyCFunction)UPnP_totalpacketsent, METH_NOARGS,
	 "return the total number of packets sent by UPnP IGD"
	},
	{"totalpacketreceived", (PyCFunction)UPnP_totalpacketreceived, METH_NOARGS,
	 "return the total number of packets received by UPnP IGD"
	},
	{"statusinfo", (PyCFunction)UPnP_statusinfo, METH_NOARGS,
	 "return status and uptime"
	},
	{"connectiontype", (PyCFunction)UPnP_connectiontype, METH_NOARGS,
	 "return IGD WAN connection type"
	},
	{"externalipaddress", (PyCFunction)UPnP_externalipaddress, METH_NOARGS,
	 "return external IP address"
	},
	{"addportmapping", (PyCFunction)UPnP_addportmapping, METH_VARARGS,
	 "add a port mapping"
	},
	{"deleteportmapping", (PyCFunction)UPnP_deleteportmapping, METH_VARARGS,
	 "delete a port mapping"
	},
	{"getportmappingnumberofentries", (PyCFunction)UPnP_getportmappingnumberofentries, METH_NOARGS,
	 "-- non standard --"
	},
	{"getspecificportmapping", (PyCFunction)UPnP_getspecificportmapping, METH_VARARGS,
	 "get details about a specific port mapping entry"
	},
	{"getgenericportmapping", (PyCFunction)UPnP_getgenericportmapping, METH_VARARGS,
	 "get all details about the port mapping at index"
	},
    {NULL}  /* Sentinel */
};

static PyTypeObject UPnPType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "miniupnpc.UPnP",          /*tp_name*/
    sizeof(UPnPObject),        /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)UPnPObject_dealloc,/*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "UPnP objects",            /* tp_doc */
    0,		                   /* tp_traverse */
    0,		                   /* tp_clear */
    0,		                   /* tp_richcompare */
    0,		                   /* tp_weaklistoffset */
    0,		                   /* tp_iter */
    0,		                   /* tp_iternext */
    UPnP_methods,              /* tp_methods */
    UPnP_members,              /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,/*(initproc)UPnP_init,*/      /* tp_init */
    0,                         /* tp_alloc */
#ifndef WIN32
    PyType_GenericNew,/*UPnP_new,*/      /* tp_new */
#else
    0,
#endif
};

/* module methods */
static PyMethodDef miniupnpc_methods[] = {
    {NULL}  /* Sentinel */
};

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initminiupnpc(void) 
{
    PyObject* m;

#ifdef WIN32
    UPnPType.tp_new = PyType_GenericNew;
#endif
    if (PyType_Ready(&UPnPType) < 0)
        return;

    m = Py_InitModule3("miniupnpc", miniupnpc_methods,
                       "miniupnpc module.");

    Py_INCREF(&UPnPType);
    PyModule_AddObject(m, "UPnP", (PyObject *)&UPnPType);
}

