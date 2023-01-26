import os

freeze("modules", "camera.py")
freeze("modules", "display.py")
freeze("modules", "microphone.py")
freeze("modules", "update.py")

include("$(MPY_DIR)/extmod/uasyncio/manifest.py")

# Always include tests unless we're doing a release build
if os.getenv('REMOVE_TESTS_FOR_RELEASE') is None:
    freeze("modules", "test.py")