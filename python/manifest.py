module("_mkfs.py", base_path="$(PORT_DIR)/../python", opt=3)
module("main.py", base_path="$(PORT_DIR)/../python", opt=3)
include("$(MPY_DIR)/extmod/uasyncio")
