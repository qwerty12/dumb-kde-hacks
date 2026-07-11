/* Note: Claude churned the initial version of this out. This is simple and contains just what I need */

package main

import (
	"os"
	"syscall"
	"time"
	"unsafe"
)

const (
	uinputIoctlBase = 'U'

	EV_SYN = 0x00
	EV_KEY = 0x01

	SYN_REPORT = 0

	KEY_LEFTSHIFT = 42
	KEY_INSERT    = 110
)

var (
	UI_DEV_CREATE  = _io(uinputIoctlBase, 1)
	UI_DEV_DESTROY = _io(uinputIoctlBase, 2)
	UI_DEV_SETUP   = _iow(uinputIoctlBase, 3, unsafe.Sizeof(uinput_setup{}))
	UI_SET_EVBIT   = _iow(uinputIoctlBase, 100, 4)
	UI_SET_KEYBIT  = _iow(uinputIoctlBase, 101, 4)
)

func _io(magic, nr uintptr) uintptr {
	return (magic << 8) | nr
}

func _iow(magic, nr, size uintptr) uintptr {
	return (1 << 30) | (size << 16) | (magic << 8) | nr
}

type input_id struct {
	Bustype uint16
	Vendor  uint16
	Product uint16
	Version uint16
}

type uinput_setup struct {
	ID             input_id
	Name           [80]byte
	FF_effects_max uint32
}

type input_event struct {
	Time  syscall.Timeval
	Type  uint16
	Code  uint16
	Value int32
}

func ioctl(fd, cmd, arg uintptr) error {
	if _, _, errno := syscall.Syscall(syscall.SYS_IOCTL, fd, cmd, arg); errno != 0 {
		return errno
	}
	return nil
}

func writeEvent(f *os.File, ev *input_event, evType, code uint16, value int32) error {
	ev.Type = evType
	ev.Code = code
	ev.Value = value

	buf := unsafe.Slice((*byte)(unsafe.Pointer(ev)), unsafe.Sizeof(*ev))
	_, err := f.Write(buf)
	return err
}

type ukbd struct {
	f *os.File
	_ noCopy
}

func uinputKbdInit() (ukbd, error) {
	f, err := os.OpenFile("/dev/uinput", os.O_WRONLY|syscall.O_NONBLOCK|syscall.O_CLOEXEC, 0)
	if err != nil {
		return ukbd{}, err
	}

	var setup uinput_setup
	fd := f.Fd()

	err = ioctl(fd, UI_SET_EVBIT, EV_SYN)
	if err != nil {
		goto errOut
	}
	err = ioctl(fd, UI_SET_EVBIT, EV_KEY)
	if err != nil {
		goto errOut
	}
	err = ioctl(fd, UI_SET_KEYBIT, KEY_LEFTSHIFT)
	if err != nil {
		goto errOut
	}
	err = ioctl(fd, UI_SET_KEYBIT, KEY_INSERT)
	if err != nil {
		goto errOut
	}

	/* XXX: change back to BUS_USB if incompatibilities arise with certain programs */
	setup.ID = input_id{Bustype: 0x06 /* BUS_VIRTUAL */, Vendor: 0x7239, Product: 0x3666, Version: 1}
	copy(setup.Name[:], "kpos-virtual-kbd")
	err = ioctl(fd, UI_DEV_SETUP, uintptr(unsafe.Pointer(&setup)))
	if err != nil {
		goto errOut
	}
	err = ioctl(fd, UI_DEV_CREATE, 0)
	if err != nil {
		goto errOut
	}
	time.Sleep(150 * time.Millisecond)

	return ukbd{f: f}, nil
errOut:
	_ = f.Close()
	return ukbd{}, err
}

func (u *ukbd) uinputKbdSendPaste() {
	var ev input_event

	if u.f == nil {
		return
	}

	f := u.f
	// press
	if writeEvent(f, &ev, EV_KEY, KEY_LEFTSHIFT, 1) == nil {
		if writeEvent(f, &ev, EV_KEY, KEY_INSERT, 1) == nil {
			_ = writeEvent(f, &ev, EV_SYN, SYN_REPORT, 0)
		}
	}

	// release
	_ = writeEvent(f, &ev, EV_KEY, KEY_INSERT, 0)
	_ = writeEvent(f, &ev, EV_KEY, KEY_LEFTSHIFT, 0)
	_ = writeEvent(f, &ev, EV_SYN, SYN_REPORT, 0)
}

func (u *ukbd) uinputKbdClose() {
	if u.f == nil {
		return
	}

	f := u.f
	u.f = nil
	_ = ioctl(f.Fd(), UI_DEV_DESTROY, 0)
	_ = f.Close()
}

type noCopy struct{}

func (*noCopy) Lock()   {}
func (*noCopy) Unlock() {}
