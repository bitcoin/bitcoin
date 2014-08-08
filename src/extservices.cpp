
#include <vector>
#include "serialize.h"
#include "util.h"
#include "version.h"
#include "net.h"
#include "extservices.h"
#include <boost/foreach.hpp>
#include "json/json_spirit_value.h"
#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_utils.h"

using namespace std;
using namespace json_spirit;

static const unsigned int MAX_JSON_SRV_SZ = (640 * 1024); // enough for anybody

class CExtServices {
private:
    vector<CExtService> srvList;

public:
    void getArray(Array& arr);
    void getVec(vector<CExtService>& vec);
    void add(const CExtService& val) { srvList.push_back(val); }
    size_t count() const { return srvList.size(); }
};

static CExtServices extServices;

bool CExtService::parseValue(const Value& val)
{
    if (val.type() != obj_type)
        return false;

    const Object& srvObj = val.get_obj();

    const Value& vName = find_value(srvObj, "name");
    if (vName.type() != str_type)
        return false;

    name = vName.get_str();

    const Value& vPort = find_value(srvObj, "port");
    if (vPort.type() == null_type) {
        // do nothing
    } else if (vPort.type() == int_type) {
        int i = vPort.get_int();
        if (i < 1 || i > 65535)
            return false;

        port = i;
    } else
        return false;

    const Value& vAttrib = find_value(srvObj, "attrib");
    if (vAttrib.type() == null_type) {
        // do nothing
    } else if (vAttrib.type() == obj_type) {
        const Object& attribObj = vAttrib.get_obj();
        BOOST_FOREACH(const Pair& s, attribObj) {
            const Value& av = s.value_;
            if (av.type() != str_type)
                return false;

            CKeyValue kv(s.name_, av.get_str());
            if (kv.key.size() < 1 || kv.key.size() > 100 ||
                kv.value.size() > 1000)
                return false;

            attrib.push_back(kv);
        }
    } else
        return false;

    return true;
}

void CExtServices::getArray(Array& arr)
{
    BOOST_FOREACH(const CExtService& srv, srvList) {
        Object obj;
        obj.push_back(Pair("name", srv.name));
        if (srv.port > 0)
            obj.push_back(Pair("port", srv.port));

        arr.push_back(obj);
    }
}

void CExtServices::getVec(vector<CExtService>& vec)
{
    BOOST_FOREACH(const CExtService& srv, srvList) {
        vec.push_back(srv);
    }
}

static bool ReadJsonFile(const string& filename, size_t maxSz,
                         Value& valOut, string& strErr)
{
    CAutoFile srvfile(fopen(filename.c_str(), "r"), SER_DISK, CLIENT_VERSION);
    if (!srvfile) {
        strErr = strprintf("Cannot open %s\n", filename);
        return false;
    }

    string rawJson;
    while (1) {
        char buf[4096];

        size_t bread = fread(buf, 1, sizeof(buf), srvfile);
        rawJson.append(buf, bread);

        if (maxSz && (rawJson.size() > maxSz)) {
            strErr = strprintf("Error reading %s, too large\n", filename);
            return false;
        }

        if (bread < sizeof(buf)) {
            if (ferror(srvfile)) {
                strErr = strprintf("Error reading %s\n", filename);
                return false;
            }

            // eof
            break;
        }
    }

    if (!read_string(rawJson, valOut) ||
        (valOut.type() != obj_type)) {
        strErr = strprintf("Error parsing JSON in %s\n", filename);
        return false;
    }

    return true;
}

static bool ReadExtServiceFile(const string& filename)
{
    Value valSrv;
    string strErr;
    if (!ReadJsonFile(filename, MAX_JSON_SRV_SZ, valSrv, strErr)) {
        LogPrintf("%s: %s", filename, strErr);
        return false;
    }

    CExtService srv;
    if (!srv.parseValue(valSrv) ||
        (valSrv.type() != array_type)) {
        LogPrintf("Unable to parse value in %s\n", filename);
        return false;
    }

    const Array& serviceList = valSrv.get_array();
    for (unsigned int i = 0; i < serviceList.size(); i++) {
        if (serviceList[i].type() != obj_type) {
            LogPrintf("Service list member not object\n");
            return false;
        }

        CExtService srv;
        if (!srv.parseValue(serviceList[i]))
            return false;

        extServices.add(srv);
    }

    return true;
}

bool ReadExtServices()
{
    if (!mapArgs.count("-extservices"))
        return true;
    if (!ReadExtServiceFile(mapArgs["-extservice"]))
        return false;

    if (extServices.count() > 0)
        nLocalServices |= NODE_EXT_SERVICES;

    return true;
}

Array ListExtServices()
{
    Array ret;
    extServices.getArray(ret);
    return ret;
}

void GetExtServicesVec(vector<CExtService>& vec)
{
    extServices.getVec(vec);
}
