.. |date| date::
.. |time| date:: %H:%M

BrilliantLabs Monocle
=====================
.. figure:: /images/monocle.png

   *Monocle.*

Augmented Reality (AR) is no longer limited to laboratories and fieldwork.
Monocle is a pocket-sized device that clips right to your glasses.
What it does best is help you collect memories.
Share your clips and photos.
Zoom in on far away objects.
All without ever having to take your phone out of your pocket.
Live life without looking down at your phone.

Visit `Monocle <https://www.itsbrilliant.co/>`_ to get hands on one.
And `Discord <https://discord.gg/3YvPv8tDMj>`_ channel for any new updates.

Audience
--------
This document is mainly for software developers to the opensource community and testers who are contributing to build firmware,
FPGA and Mobile application for the monocle product.

Future Changes to Monocle
-------------------------

Monocle Firmware Side
^^^^^^^^^^^^^^^^^^^^^
* Audio transfer from Monocle Hardware to Phone Application
* Reliable transfer of data to phone
* Data tranfer from Phone to Monocle Hardware
* FPGA Upgrade feature

Monocle FPGA Side
^^^^^^^^^^^^^^^^^
* Video Compression for faster Video transfer from Monocle to Phone

Monocle Phone Application Side
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
* Application on phone displaying Video/Photo transferred from monocle

.. toctree::
   :maxdepth: 5
   :caption: Firmware

   firmware_how_to.rst
   firmware_architecture.rst
   firmware_driver_api/html/index.rst
   firmware_python_api/index.rst
   firmware_release_notes.rst

.. toctree::
   :maxdepth: 5
   :caption: FPGA

   fpga_how_to.rst

.. toctree::
   :maxdepth: 5
   :caption: Hardare

   hardware_how_to.rst
   hardware_architecture.rst

.. toctree::
   :maxdepth: 5
   :caption: Phone App

   phone_app_how_to.rst
   phone_app_architecture.rst
   phone_app_release_notes.rst
