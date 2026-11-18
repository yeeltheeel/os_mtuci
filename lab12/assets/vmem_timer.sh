#!/bin/bash
set -euo pipefail

TIMER="vmemtest.timer"
SERVICE="vmemtest.service"
TARGET="vmem.sh"
UNIT_DIR="/etc/systemd/system"
SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"

ACTION="${1:-start}"

if [[ $EUID -ne 0 ]]; then
    echo "root required: sudo $0 ${ACTION}" >&2
    exit 1
fi

# all three files must be next to this script 
for f in "$TIMER" "$SERVICE" "$TARGET"; do
    if [[ ! -f "$SCRIPT_DIR/$f" ]]; then
        echo "error: $SCRIPT_DIR/$f not found" >&2
        echo "place $TIMER, $SERVICE and $TARGET in the same directory as this script" >&2
        exit 1
    fi
done

if [[ ! -x "$SCRIPT_DIR/$TARGET" ]]; then
    echo ">>> chmod +x $TARGET"
    chmod +x "$SCRIPT_DIR/$TARGET"
fi

install_units() {
    local changed=0

    local tmp
    tmp="$(mktemp)"
    sed -E "s|^ExecStart=.*|ExecStart=$SCRIPT_DIR/$TARGET|" \
        "$SCRIPT_DIR/$SERVICE" > "$tmp"

    if ! cmp -s "$tmp" "$UNIT_DIR/$SERVICE"; then
        echo ">>> installing $SERVICE -> $UNIT_DIR/  (ExecStart=$SCRIPT_DIR/$TARGET)"
        install -m 0644 "$tmp" "$UNIT_DIR/$SERVICE"
        changed=1
    fi
    rm -f "$tmp"

    if ! cmp -s "$SCRIPT_DIR/$TIMER" "$UNIT_DIR/$TIMER"; then
        echo ">>> installing $TIMER -> $UNIT_DIR/"
        install -m 0644 "$SCRIPT_DIR/$TIMER" "$UNIT_DIR/$TIMER"
        changed=1
    fi

    if (( changed )); then
        echo ">>> systemd reload..."
        systemctl daemon-reload
    fi
}

case "$ACTION" in
    start)
        install_units

        echo ">>> enable --now $TIMER..."
        systemctl enable --now "$TIMER"

        echo
        systemctl status "$TIMER" --no-pager || true

        echo
        systemctl list-timers "$TIMER" --no-pager || true

        echo
        echo ">>> waiting (10s)..."
        sleep 11

        echo
        systemctl status "$SERVICE" --no-pager || true
        ;;

    stop)
        echo ">>> disable --now $TIMER..."
        systemctl disable --now "$TIMER" || true
        echo "done"
        ;;

    status)
        systemctl status "$TIMER" --no-pager || true
        echo
        systemctl status "$SERVICE" --no-pager || true
        echo
        systemctl list-timers "$TIMER" --no-pager || true
        ;;

    logs)
        echo ">>> logs $SERVICE (ctrl+c for exit)..."
        journalctl -u "$SERVICE" -f
        ;;

    *)
        echo "usage: $0 {start|stop|status|logs}" >&2
        exit 1
        ;;
esac
