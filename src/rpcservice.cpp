#include "main.h"
#include "db.h"
#include "init.h"
#include "rpcserver.h"

#include <fstream>
using namespace json_spirit;
using namespace std;

Value service(const Array& params, bool fHelp)
{
    string strCommand;
    if (params.size() >= 1)
        strCommand = params[0].get_str();

    if (fHelp  ||
        (strCommand != "enroll" && strCommand != "unenroll" && strCommand != "genservicekey" && strCommand != "addservicekey" && strCommand != "removeservicekey" && strCommand != "status" && strCommand != "getcurrentclients" && strCommand != "setprice" && strCommand != "getprice" && strCommand != "sendclientdata" && strCommand != "discover"))
        throw runtime_error(
                "service \"command\"... ( \"passphrase\" )\n"
                "Vote or show current budgets\n"
                "\nAvailable commands:\n"
                "  enroll               - Enroll to a service on the network\n"
                "  unenroll             - Un-enroll from a service on the network\n"
                "  genservicekey        - Generate a key to run a service on the network\n"
                "  addservicekey        - Add an already existing service key \n"
                "  removeservicekey     - Remove a service key\n"
                "  status               - Show the status of a service\n"
                "  getcurrentclients    - Show current clients subscribed to your service\n"
                "  setprice             - Set the price for your service\n"
                "  getprice             - Get the price for the service hash\n"
                "  sendclientdata       - Manually send client data\n"
                "  discover             - List of all services on the network\n"
                );

    if(strCommand == "enroll")
    {
        // Registers a service on the network, this is called when you launch a service module
        if (params.size() != 5)
            throw runtime_error("Correct usage is 'service enroll serviceKey sharemodel price timeInterval'");
        return Value::null;
    }

    if(strCommand == "unenroll")
    {
        // used when stopping the service. Although the network will ping the service at least once per hour
        // and if service does not respond, it will be unregistered by the network.
        if (params.size() != 2)
            throw runtime_error("Correct usage is 'service unenroll serviceKey'");
        return Value::null;
    }

    if(strCommand == "genservicekey")
    {
        // Generate a key to run a service on the network
        if (params.size() != 1)
            throw runtime_error("Correct usage is 'service genservicekey'");
        return Value::null;
    }

    if(strCommand == "addservicekey")
    {
        // 
        if (params.size() != 2)
            throw runtime_error("Correct usage is 'service addservicekey serviceKey'");
        return Value::null;
    }

    if(strCommand == "removeservicekey")
    {
        // 
        if (params.size() != 2)
            throw runtime_error("Correct usage is 'service removeservicekey serviceKey'");
        return Value::null;
    }

    if(strCommand == "status")
    {
        // This will return current status of a service – serviceKey is used for this
        if (params.size() != 2)
            throw runtime_error("Correct usage is 'service status serviceKey'");
        return Value::null;
    }

    if(strCommand == "getcurrentclients")
    {
        // this will return a list of uniqely identifyable clients that have currently bought services
        if (params.size() != 2)
            throw runtime_error("Correct usage is 'service getcurrentclients serviceKey'");
        return Value::null;
    }

    if(strCommand == "setprice")
    {
        // Set the service price - serviceKey is used for this
        if (params.size() != 2)
            throw runtime_error("Correct usage is 'service setprice serviceKey'");
        return Value::null;
    }

    if(strCommand == "getprice")
    {
        // Get the service price - serviceKey is used for this
        if (params.size() != 2)
            throw runtime_error("Correct usage is 'service getprice serviceKey'");
        return Value::null;
    }

    if(strCommand == "sendclientdata")
    {
        // Manually send client data
        return Value::null;
    }

    if(strCommand == "discover")
    {
        // Generates a list of services currently running on the network
        if (params.size() != 1)
            throw runtime_error("Correct usage is 'service discover'");
        return Value::null;
    }

    return Value::null;
}
