#!/bin/bash

set -euo pipefail

TIMER="vmemtest.timer"
SERVICE="vmemtest.service"

if [[ $EUID -ne 0 ]]; then
    echo "root required (sudo ./vmem_timer.sh)" >&2
    exit 1
fi

if [[ ! -f "/etc/systemd/system/$TIMER" ]] || [[ ! -f "/etc/systemd/system/$SERVICE" ]]; then
    echo "error: /etc/systemd/system/$TIMER или $SERVICE not found" >&2
    echo "create them first" >&2
    exit 1
fi

ACTION="${1:-start}"

case "$ACTION" in
    start)
        echo ">>> systemd reload..."
        systemctl daemon-reload

        echo ">>> enable $TIMER..."
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
        echo ">>> disable $TIMER..."
        systemctl disable --now "$TIMER"
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