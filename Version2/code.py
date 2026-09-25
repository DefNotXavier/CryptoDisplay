import time
import usb_hid
from adafruit_hid.keyboard import Keyboard
from adafruit_hid.keyboard_layout_us import KeyboardLayoutUS
from adafruit_hid.keycode import Keycode

# Wait for USB enumeration and drive mount before typing. Raise this if the
# chip is plugged in at power-on and nothing types (host desktop not up yet).
BOOT_SETTLE_SECONDS = 5

# Wait after opening the Run box / terminal before typing into it.
WINDOW_OPEN_SECONDS = 2

kbd = Keyboard(usb_hid.devices)
layout = KeyboardLayoutUS(kbd)


def type_line(text):
    layout.write(text)
    kbd.press(Keycode.ENTER)
    kbd.release_all()


# Find the drive by label (drive letter / mount path varies) and run its launcher.
CMD_WINDOWS = (
    'powershell -ExecutionPolicy Bypass -Command '
    '"& ((Get-Volume -FileSystemLabel CRYPTOS).DriveLetter + \':\\launch.ps1\')"'
)
CMD_LINUX = 'bash "$(findmnt -n -o TARGET LABEL=CRYPTOS)/launch.sh"'

time.sleep(BOOT_SETTLE_SECONDS)

# Win+D first so a Windows AutoPlay Explorer window can't steal the keystrokes.
kbd.press(Keycode.GUI, Keycode.D)
kbd.release_all()
time.sleep(1)

# Windows: Win+R. Does nothing on most Linux desktops.
kbd.press(Keycode.GUI, Keycode.R)
kbd.release_all()
time.sleep(WINDOW_OPEN_SECONDS)
type_line(CMD_WINDOWS)
time.sleep(WINDOW_OPEN_SECONDS)

# Linux: Ctrl+Alt+T. Unbound on Windows; not bound on every Linux desktop.
kbd.press(Keycode.CONTROL, Keycode.ALT, Keycode.T)
kbd.release_all()
time.sleep(WINDOW_OPEN_SECONDS)
type_line(CMD_LINUX)
