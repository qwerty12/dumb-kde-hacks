package main

import (
	"context"
	"fmt"
	"os"
	"os/signal"
	"syscall"
	"time"

	"github.com/godbus/dbus/v5"
)

const (
	// Max gap allowed between clipboardHistoryUpdated firing and the popup
	// closing for that update to count as "the user picked something".
	pasteWindow = 1 * time.Second

	// KWin signals before the new window actually has focus.
	// Wait an arbitrary amount of time before sending the key
	// combo to try to ensure the right app gets it in its entirety.
	pasteDelay = 150 * time.Millisecond
)

var popupEvents = make(chan bool, 8)

type ww struct{}

type clipboardPasteState struct {
	popupOpen         bool
	updatedWhilePopup bool
	updatedAt         time.Time
}

func (s *clipboardPasteState) onPopupActivated(active bool) (shouldPaste bool) {
	switch {
	case active && !s.popupOpen:
		s.updatedWhilePopup = false

	case !active && s.popupOpen:
		if s.updatedWhilePopup && time.Since(s.updatedAt) <= pasteWindow {
			shouldPaste = true
		}
		s.updatedWhilePopup = false
	}

	s.popupOpen = active
	return shouldPaste
}

func (s *clipboardPasteState) onClipboardHistoryUpdated() {
	if s.popupOpen {
		s.updatedWhilePopup = true
		s.updatedAt = time.Now()
	}
}

func (w ww) WindowActivated(isKlipperPopup bool) *dbus.Error {
	select {
	case popupEvents <- isKlipperPopup:
	default:
	}
	return nil
}

func main() {
	conn, err := dbus.ConnectSessionBus()
	if err != nil {
		_, _ = fmt.Fprintln(os.Stderr, err)
		return
	}
	defer conn.Close()

	w := ww{}
	err = conn.Export(w, "/", "com.q12.kpos.WindowWatcher")
	if err != nil {
		_, _ = fmt.Fprintln(os.Stderr, err)
		return
	}
	reply, err := conn.RequestName("com.q12.kpos", dbus.NameFlagDoNotQueue)
	if err != nil {
		_, _ = fmt.Fprintln(os.Stderr, err)
		return
	}
	if reply != dbus.RequestNameReplyPrimaryOwner {
		_, _ = fmt.Fprintln(os.Stderr, "name already taken")
		return
	}

	if err = conn.AddMatchSignal(
		dbus.WithMatchSender("org.kde.klipper"),
		dbus.WithMatchObjectPath("/klipper"),
		dbus.WithMatchInterface("org.kde.klipper.klipper"),
		dbus.WithMatchMember("clipboardHistoryUpdated"),
	); err != nil {
		_, _ = fmt.Fprintln(os.Stderr, err)
		return
	}
	c := make(chan *dbus.Signal, 10)
	conn.Signal(c)

	u, err := uinputKbdInit()
	if err != nil {
		_, _ = fmt.Fprintln(os.Stderr, err)
		return
	}
	defer u.uinputKbdClose()

	ctx, stop := signal.NotifyContext(context.Background(), syscall.SIGINT, syscall.SIGTERM)
	defer stop()

	state := &clipboardPasteState{}
	pasteRequest := make(chan struct{}, 1)

	for {
		select {
		case /*v :=*/ <-c:
			//if v.Name == "org.kde.klipper.klipper.clipboardHistoryUpdated" {
			state.onClipboardHistoryUpdated()
			//}

		case isOpen := <-popupEvents:
			if state.onPopupActivated(isOpen) {
				go func() {
					select {
					case <-time.After(pasteDelay):
					case <-ctx.Done():
						return
					}
					select {
					case pasteRequest <- struct{}{}:
					default:
					}
				}()
			}

		case <-pasteRequest:
			if !state.popupOpen {
				u.uinputKbdSendPaste()
			}

		case <-ctx.Done():
			return
		}
	}
}
