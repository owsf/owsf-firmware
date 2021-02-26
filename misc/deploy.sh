#!/bin/bash

if [[ -z "$OWSF_SERVER_URL" ]]; then
    echo "OWSF_SERVER_URL is not set"
    exit -1
fi

if [[ -z "$IOTA_TOKEN" ]]; then
    echo "IOTA_TOKEN is not set"
    exit -1
fi

do_upload_local_config() {
    c="$1"
    if [[ ! -f "$c" ]]; then
        echo "$c not found"
        return -1
    fi
    chip=$(basename $c | awk -F '.' '{print $3}')
    curl -v -X PUT -H 'Content-Type: application/json' \
        -H "X-auth-token: $IOTA_TOKEN"\
        -H "X-chip-id: $chip" \
        -d "$(cat $c)" \
        https://$OWSF_SERVER_URL/api/v1/deploy/local_config
}

case "$1" in
    "global_config")
        GLOBAL_CONFIG_FILE=${2:-$DATA_DIR/global_config.json}
        if [[ ! -f "$GLOBAL_CONFIG_FILE" ]]; then
            echo "$GLOBAL_CONFIG_FILE not found"
            exit -1
        fi

        GCK=$(jq -r .global_config_key $GLOBAL_CONFIG_FILE)
        GLOBAL_CONFIG_KEY=${GLOBAL_CONFIG_KEY:-$GCK)}

        if [[ -z "$GLOBAL_CONFIG_KEY" ]]; then
            echo "GLOBAL_CONFIG_KEY is not set"
            exit -1
        fi

        curl -v -X PUT -H 'Content-Type: application/json' \
            -H "X-auth-token: $IOTA_TOKEN" \
            -H "X-global-config-key: $GLOBAL_CONFIG_KEY" \
            -d "$(cat $GLOBAL_CONFIG_FILE)"\
            https://$OWSF_SERVER_URL/api/v1/deploy/global_config
        ;;

    "firmware")
        FF=$(ls "$DATA_DIR/firmware*sig*" | sort | tail -n 1)
        FIRMWARE_FILE=${2:-$FF}

        if [[ ! -f "$FIRMWARE_FILE" ]]; then
            echo "$FIRMWARE_FILE not found"
            exit -1
        fi

        FV=$(basename $FIRMWARE_FILE | \
            sed -E -e 's/firmware_?//g' -e 's/\.sig\.?//g' -e 's/v//g')

        FIRMWARE_VERSION=${FIRMWARE_VERSION:-$FV}

        if [[ -z "$FIRMWARE_VERSION" ]]; then
            echo "FIRMWARE_VERSION is not set"
            exit -1
        fi

        TMPFILE="$(mktemp)"
        base64 "$FIRMWARE_FILE" > "$TMPFILE"
        curl -v -X PUT -H "Content-Type: text/plain" \
            -H "X-auth-token: $IOTA_TOKEN" \
            -H "X-firmware-version: $FIRMWARE_VERSION" \
            --upload-file "$TMPFILE" \
            https://$OWSF_SERVER_URL/api/v1/deploy/firmware
        rm -rf "$TMPFILE"
        ;;

    "local_config")
        if [[ -z "$2" ]]; then
            for c in $(ls $DATA_DIR/config.json.*); do
                do_upload_local_config "$c"
            done
        else
            do_upload_local_config "$2"
        fi
        ;;

    *)
        echo "usage"
        echo "  $0 global_config|firmware|local_config"
        ;;
esac
