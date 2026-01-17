Alternative networking over satellite and 4G
============================================

This guide provides operational safety recommendations for running Bitcoin Core
when using satellite, cellular (4G/LTE/5G), or other constrained and potentially
observable network links. It focuses on reducing address leakage and limiting
inbound exposure while still allowing the node to operate.

Scope and threat model
----------------------

Satellite and 4G links can be monitored by intermediaries (ISP, carrier, or
satellite operator). Compared to typical broadband, these links can also be
asymmetric, expensive, and have high latency. The goals here are to:

- Reduce the chance that your public IP address or location is exposed to peers.
- Prevent unwanted inbound connections over a potentially expensive link.
- Prefer privacy-preserving network overlays when available.

Prefer Tor/I2P/CJDNS when possible
---------------------------------

Whenever possible, route Bitcoin Core traffic through privacy-focused networks:

- **Tor** hides your public IP from peers and allows inbound connectivity via an
  onion service.
- **I2P** provides a similar privacy model with inbound/outbound tunnels.
- **CJDNS** provides end-to-end encryption and private addressing on a mesh.

When these overlays are available, prefer using them for all P2P traffic and
avoid direct IPv4/IPv6 connections.

Recommended configuration patterns
----------------------------------

The following options are commonly used to reduce exposure and limit which
networks are used. Combine them based on your setup.

### 1. Force only privacy networks

Use `-onlynet` to restrict which network types are used for outbound and inbound
connections.

Examples:

- Tor only:
  - `-onlynet=onion`
- I2P only:
  - `-onlynet=i2p`
- CJDNS only:
  - `-onlynet=cjdns`

This prevents accidental direct IPv4/IPv6 connections over a satellite or 4G
link.

### 2. Use a proxy for outbound traffic

Use `-proxy` to ensure all outbound traffic goes through a proxy (typically
Tor). For example:

- `-proxy=127.0.0.1:9050`

If you also want DNS lookups to happen through the proxy, set:

- `-proxy=127.0.0.1:9050`
- `-onion=127.0.0.1:9050`

(See [Tor Support](tor.md) for more detailed guidance.)

### 3. Disable inbound connections on expensive or exposed links

Use `-listen=0` to disable inbound connections if you only want outbound
connectivity. This is often appropriate for metered or high-latency links.

Example:

- `-listen=0`

If you still want inbound connections but only on a specific interface, use
`-bind` to limit the address. For example, when using a local-only interface:

- `-bind=127.0.0.1`

### 4. Bind explicitly when multiple interfaces are present

Satellite or cellular modems sometimes add multiple interfaces (e.g., NAT, VPN,
local tether). Use `-bind` to restrict listening to a specific address, and
combine with `-listen=1` only when you truly want inbound peers.

Example:

- `-bind=10.0.0.5` (example private interface)
- `-listen=1`

Warnings about address leakage
------------------------------

- **Direct IPv4/IPv6 connections reveal your public IP** to peers and can be
  correlated with your satellite/cellular provider or geography.
- **Accidental interface exposure** can occur if you leave `-listen=1` with a
  public interface and do not restrict it with `-bind`.
- **Mixed-network setups** can still leak if `-onlynet` is not set and fallback
  to IPv4/IPv6 is allowed.

Operational checklist
---------------------

- Prefer `-onlynet=onion`/`i2p`/`cjdns` when possible.
- Use `-proxy` to route all outbound traffic through Tor/I2P.
- If you do not need inbound peers, set `-listen=0`.
- If you do need inbound peers, use `-bind` to limit the interface.
- Avoid mixed direct and privacy-overlay connections unless you accept the risk
  of public IP exposure.

Related documentation
---------------------

- [Tor Support](tor.md)
- [I2P Support](i2p.md)
- [CJDNS Support](cjdns.md)
- [bitcoin.conf Configuration File](bitcoin-conf.md)
