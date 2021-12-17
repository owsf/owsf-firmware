for c in $(ls server_data/config.json*); do
    chip=$(echo $c | awk -F '.' '{print $3}')
    curl -v -H 'Content-Type: application/json' -H "X-auth-token: $IOTA_TOKEN" \
        -H "X-chip-id: $chip" -d "$(cat $c)" \
        -X PUT $IOTA_URL/api/v1/deploy/local_config;
done

curl -v -H 'Content-Type: application/json' -H "X-auth-token: $IOTA_TOKEN" \
    -H "X-global-config-key: $(cat data/global_config.json | jq -M -r .global_config_key)" \
    -X PUT -d "$(cat data/global_config.json)" $IOTA_URL/api/v1/deploy/global_config

curl -v -H "Content-Type: text/plain" -H "X-auth-token: $IOTA_TOKEN" \
    -H "X-firmware-version: $(cat server_data/firmware.json | jq -M -r .version)" \
    -X PUT -d "@server_data/$(cat server_data/firmware.json | jq -M -r .file)" $IOTA_URL/api/v1/deploy/firmware
