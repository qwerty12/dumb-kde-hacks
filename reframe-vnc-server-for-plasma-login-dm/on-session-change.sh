#!/bin/sh

case "$PAM_TYPE" in
  open_session)
    systemctl stop reframe-server@
    ;;
  close_session)
    closing="$XDG_SESSION_ID"
    still_active=0
    for s in $(loginctl list-sessions --no-legend | awk '{print $1}'); do
      [ "$s" = "$closing" ] && continue # ignore the session that's ending
      class=$(loginctl show-session "$s" -p Class --value 2>/dev/null)
      [ "$class" = "user" ] && { still_active=1; break; }
    done
    [ "$still_active" -eq 0 ] && systemctl start reframe-server@
    ;;
esac
