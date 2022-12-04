:py:mod:`machine`
-----------------

🟠 means "not implemented yet"


.. py:module:: machine


.. py:class:: Battery() 🟠

   Sensing battery power.

   .. py:method:: level() 🟠

      :return: the current battery level as a percentage
   

.. py:class:: Camera() 🟠

   Controlling feeds from the camera.

   .. py:method:: capture() 🟠

      Sends the last captured image over Bluetooth
   
   .. py:method:: stream() 🟠

      Starts streaming images over Bluetooth.
      Because Bluetooth is involved, this will have a low frame rate.
   
   .. py:method:: stop() 🟠

      Stops any ongoing image stream or capture currently in progress

.. py:class:: Display() 🟠

   Drawing to the display.

   .. py:method:: show(framebuffer) 🟠

      Pushes a framebuffer object to the FPGA for drawing onto the display


.. py:class:: FPGA() 🟠

   Communication with the FPGA.

   .. py:method:: download() 🟠

      Downloads and reboots the FPGA with the bitstream from Bluetooth.
      Automatically powers up the FPGA if it was powered down.

   .. py:method:: spi_read(cmd, addr, bytes) 🟠

      :param int cmd: the command
      :return: a byte array

      Reads bytes from the FPGA with a given command and address.

   .. py:method:: spi_write(cmd, addr, bytes, buffer) 🟠

      Writes n bytes from buffer to the FPGA with a given command and address. 

   .. py:method:: status() 🟠

      :return: the current status of the FPGA


.. py:class:: Microphone() 🟠

   Controlling feeds fromthe microphone.

   .. py:method:: stream() 🟠

      Starts streaming audio data over Bluetooth

   .. py:method:: stop() 🟠

      Stops any ongoing audio stream


.. py:class:: Power() 🟠

   Controlling general power.

   .. py:method:: hibernate(enable) 🟠

      Enables or disables all the high power devices. Networking remains active. Upon re-enabling the FPGA will remain in reset until booted using FPGA.boot()

   .. py:method:: reset() 🟠

      Resets the device

   .. py:method:: reset_cause() 🟠

      :return: the reason for the previous reset or startup state

   .. py:method:: shutdown(timeout) 🟠

      Places the device into deep-sleep and powers down all high power devices.
      If a timeout is given, the device will wake up again after that many seconds, otherwise the device will only wake up upon inserting, and removing from the case.
      Upon wakeup, the device will reset, and the cause can be seen using the Power.reset_cause() function.


.. py:class:: Timer(id, period, callback, oneshot) 🟠

   Creates a new Timer object on timer id with the period in milliseconds and a given callback handler.
   The oneshot value can optionally be set to true if only a single trigger is required.
   By default the timer is repeating

   .. py:method:: value() 🟠

      :return: the current count value of the timer in milliseconds

   .. py:method:: deinit() 🟠

      De-initializes the timer and stops any callbacks


.. py:class:: Touch() 🟠

   Setting up touch event callbacks

.. py:function:: mac_address() 🟠

   :return: the 48bit MAC address of the device as a 17 character string. Each byte is delimited with a colon

.. py:function:: update(start) 🟠

   Checks for firmware updates and returns True if it is available.
   If start is set to True, the update process is begun, and the device will enter the bootloader state
