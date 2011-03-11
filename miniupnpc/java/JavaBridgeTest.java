import java.nio.ByteBuffer;
import fr.free.miniupnp.*;

/**
 *
 * @author syuu
 */
public class JavaBridgeTest {
    public static void main(String[] args) {
        int UPNP_DELAY = 2000;
        MiniupnpcLibrary miniupnpc = MiniupnpcLibrary.INSTANCE;
        UPNPDev devlist = null;
        UPNPUrls urls = new UPNPUrls();
        IGDdatas data = new IGDdatas();
        ByteBuffer lanaddr = ByteBuffer.allocate(16);
        ByteBuffer intClient = ByteBuffer.allocate(16);
        ByteBuffer intPort = ByteBuffer.allocate(6);
        int ret;
        int i;

        if(args.length < 2) {
            System.err.println("Usage : java [...] JavaBridgeTest port protocol");
            System.out.println("  port is numeric, protocol is TCP or UDP");
            return;
        }

        devlist = miniupnpc.upnpDiscover(UPNP_DELAY, (String) null, (String) null, 0);
        if (devlist != null) {
            System.out.println("List of UPNP devices found on the network :");
            for (UPNPDev device = devlist; device != null; device = device.pNext) {
                System.out.println("desc: " + device.descURL.getString(0) + " st: " + device.st.getString(0));
            }
            if ((i = miniupnpc.UPNP_GetValidIGD(devlist, urls, data, lanaddr, 16)) != 0) {
                switch (i) {
                    case 1:
                        System.out.println("Found valid IGD : " + urls.controlURL.getString(0));
                        break;
                    case 2:
                        System.out.println("Found a (not connected?) IGD : " + urls.controlURL.getString(0));
                        System.out.println("Trying to continue anyway");
                        break;
                    case 3:
                        System.out.println("UPnP device found. Is it an IGD ? : " + urls.controlURL.getString(0));
                        System.out.println("Trying to continue anyway");
                        break;
                    default:
                        System.out.println("Found device (igd ?) : " + urls.controlURL.getString(0));
                        System.out.println("Trying to continue anyway");

                }
                System.out.println("Local LAN ip address : " + new String(lanaddr.array()));
                ByteBuffer externalAddress = ByteBuffer.allocate(16);
                miniupnpc.UPNP_GetExternalIPAddress(urls.controlURL.getString(0),
                        new String(data.first.servicetype), externalAddress);
                System.out.println("ExternalIPAddress = " + new String(externalAddress.array()));
                ret = miniupnpc.UPNP_AddPortMapping(
                        urls.controlURL.getString(0), // controlURL
                        new String(data.first.servicetype), // servicetype
                        args[0], // external Port
                        args[0], // internal Port
                        new String(lanaddr.array()), // internal client
                        "added via miniupnpc/JAVA !", // description
                        args[1], // protocol UDP or TCP
                        null); // remote host (useless)
                if (ret != MiniupnpcLibrary.UPNPCOMMAND_SUCCESS)
                    System.out.println("AddPortMapping() failed with code " + ret);
                ret = miniupnpc.UPNP_GetSpecificPortMappingEntry(
                        urls.controlURL.getString(0), new String(data.first.servicetype),
                        args[0], args[1], intClient, intPort);
                if (ret != MiniupnpcLibrary.UPNPCOMMAND_SUCCESS)
                    System.out.println("GetSpecificPortMappingEntry() failed with code " + ret);
                System.out.println("InternalIP:Port = " +
                        new String(intClient.array()) + ":" + new String(intPort.array()));
                ret = miniupnpc.UPNP_DeletePortMapping(
                        urls.controlURL.getString(0),
                        new String(data.first.servicetype),
                        args[0], args[1], null);
                if (ret != MiniupnpcLibrary.UPNPCOMMAND_SUCCESS)
                    System.out.println("DelPortMapping() failed with code " + ret);
                miniupnpc.FreeUPNPUrls(urls);
            } else {
                System.out.println("No valid UPNP Internet Gateway Device found.");
            }
            miniupnpc.freeUPNPDevlist(devlist);
        } else {
            System.out.println("No IGD UPnP Device found on the network !\n");
        }
    }
}
