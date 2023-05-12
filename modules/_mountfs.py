import os, device

bdev = device.Storage()

try:
    os.mount(bdev, "/")
except OSError:
    os.VfsLfs2.mkfs(bdev)
    os.mount(bdev, "/")

del os
del device
del bdev
