for c in $(ls server_data/config.json*); do
    chip=$(echo $c | awk -F '.' '{print $3}')
    curl -H 'Content-Type: application/json' -H "X-auth-token: $IOTA_TOKEN" \
        -H "X-chip-id: $chip" -d "$(cat $c)" \
        -X PUT $IOTA_URL/api/v1/deploy/local_config;
done

curl -H 'Content-Type: application/json' -H "X-auth-token: $IOTA_TOKEN" \
    -H "X-global-config-key: $(cat data/global_config.json | jq -M -r .global_config_key)" \
    -X PUT -d "$(cat data/global_config.json)" $IOTA_URL/api/v1/deploy/global_config

FW_VERSION=$(cat server_data/firmware.json | jq -M -r .version)
FW_FILE="server_data/$(cat server_data/firmware.json | jq -M -r .file)"
base64 $FW_FILE | curl -H "Content-Type: text/plain" -H "X-auth-token: $IOTA_TOKEN" \
    -H "X-firmware-version: $FW_VERSION" \
    -X PUT --data-binary "@-" $IOTA_URL/api/v1/deploy/firmware
