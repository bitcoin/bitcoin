#!/bin/sh
set -e

grep -v "HiddenService" /etc/tor/torrc > /tmp/torrc.new
cat /tmp/torrc.new > /etc/tor/torrc
rm /tmp/torrc.new

create_hidden_service() {
    local service_name=$1
    shift
    local entries="$@"

    local service_dir="/var/lib/tor/${service_name}"
    mkdir -p "$service_dir"

    if [ -f "${service_dir}/private_key" ] || [ -f "${service_dir}/hs_ed25519_secret_key" ]; then
        echo "Found existing keys for hidden service: $service_name"
    fi

    chown -R tor:tor "$service_dir"
    chmod -R 700 "$service_dir"

    echo "HiddenServiceDir $service_dir" >> /etc/tor/torrc

    for entry in $entries; do
        local host=$(echo "$entry" | cut -d: -f1)
        local target_port=$(echo "$entry" | cut -d: -f2)
        local virtual_port=$(echo "$entry" | cut -d: -f3)

        # If not has virtual_port, uses target_port
        if [ -z "$virtual_port" ]; then
            virtual_port="$target_port"
        fi

        echo "HiddenServicePort $virtual_port $host:$target_port" >> /etc/tor/torrc
        echo "Configured $service_name: $host:$target_port -> $virtual_port"
    done
}

# Proccess HS_* envs
env | grep ^HS_ | while IFS= read -r var; do
    service_name=$(echo "$var" | cut -d= -f1 | sed 's/^HS_//')
    entries=$(echo "$var" | cut -d= -f2-)

    create_hidden_service "$service_name" $entries
done

# Permissions
chown -R tor:tor /var/lib/tor
chmod -R 700 /var/lib/tor

print_onion_addresses() {
    sleep 10
    echo "======== TOR HIDDEN SERVICES ========"
    for dir in /var/lib/tor/*/; do
        if [ -f "${dir}hostname" ]; then
            service_name=$(basename "$dir")
            onion_address=$(cat "${dir}hostname")
            echo "$service_name: $onion_address"
        fi
    done
    echo "===================================="
}
print_onion_addresses &

if [ "$1" = "tor" ]; then
    shift
    exec su-exec tor tor "$@"
else
    exec "$@"
fi

exec su-exec tor tor -f /etc/tor/torrc
