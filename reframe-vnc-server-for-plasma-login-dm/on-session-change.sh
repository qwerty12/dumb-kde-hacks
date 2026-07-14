#!/bin/sh

case "$PAM_TYPE" in
  open_session)
    systemd-run --collect --no-block --quiet /usr/bin/systemctl stop reframe-server@
    ;;
  close_session)
    systemd-run --collect --no-block --quiet -E XDG_SESSION_ID="$XDG_SESSION_ID" /bin/sh -c '
      still_active=0
      for s in $(loginctl list-sessions --no-legend | awk "{print \$1}"); do
        [ "$s" = "$XDG_SESSION_ID" ] && continue
        class=$(loginctl show-session "$s" -p Class --value 2>/dev/null)
        [ "$class" = "user" ] && { still_active=1; break; }
      done
      [ "$still_active" -eq 0 ] && systemctl start reframe-server@
    '
    ;;
esac
