import bluetooth
import fpga
import time

def capture(url):
  """
  Captures a single image from the camera, and sends it to a url provided. The
  function first tries to send over WiFi. If it cannot reach the internet, it
  will attempt to return it over Bluetooth.
  """
  
  # Start capture using the command 0x5004
  fpga.write(0x5004, [])

  while True:
  
    # Read the bytes remaining in the fifo
    length_array = fpga.read(0x5000, 2)
    length = (length_array[0]<<8)+length_array[1] & 0x0FFF

    if length == 0:
      return

    if length > bluetooth.max_length():
      length = bluetooth.max_length()

    buffer = fpga.read(0x5010, length)

    bluetooth.send(buffer)

def power(power_on):
  return NotImplemented