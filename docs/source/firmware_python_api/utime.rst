:py:mod:`utime`
---------------

.. py:module:: utime

.. py:function:: gmtime(epoch)

  :returns: a tuple containing the time and data according to GMT. If the epoch argument is provided, the epoch timestamp is used, other the internal time of the device

.. py:function:: localtime(epoch)

  :returns: a tuple containing the time and data according to local time. If the epoch argument is provided, the epoch timestamp is used, other the internal time of the device

.. py:function:: mktime

  The inverse of gmtime().
  :param timespec
  :type: tuple
  :returns: an epoch seconds value from the 9-tuple given

.. py:function:: time()

  :returns: the number of seconds since the epoch. If the true time hasn't been set, this will be the number of seconds since power on

.. py:function:: set_time(secs)

  Set the real time clock to a given value in seconds from the epoch

.. py:function:: sleep(secs)

  Sleep for a given number of seconds

.. py:function:: sleep_ms(msecs)

  Sleep for a given number of milliseconds

.. py:function:: sleep_us(usecs)

  Sleep for a given number of microseconds

.. py:function:: ticks_ms()

  Returns the time in ms since power on

.. py:function:: ticks_us()

  Returns the time in us since power on

