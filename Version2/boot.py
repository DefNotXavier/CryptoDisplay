import storage
import supervisor

# Launchers find the drive by this label.
m = storage.getmount("/")
m.label = "CRYPTOS"

# Host writes to the drive (logs, config.js) would otherwise restart code.py
# and re-run the launch sequence. Must be set here, not in code.py.
try:
    supervisor.runtime.autoreload = False
except AttributeError:
    supervisor.disable_autoreload()  # older CircuitPython
