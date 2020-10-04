|CI| |release| |license| |api| |wiki| |stackoverflow| |twitter| |irc| |matrix|

.. code-block::

                          ZZZZZZ
                        ZZZZZZZZZZZZ
                      ZZZZZZZZZZZZZZZZ
                     ZZZZZZZ     ZZZZZZ
                    ZZZZZZ        ZZZZZ
                    ZZZZZ          ZZZZ
                    ZZZZ           ZZZZZ
                    ZZZZ           ZZZZ
                    ZZZZ          ZZZZZ
                    ZZZZ        ZZZZZZ
                    ZZZZ     ZZZZZZZZ       777        7777       7777777777
              ZZ    ZZZZ   ZZZZZZZZ         777      77777777    77777777777
          ZZZZZZZ   ZZZZ  ZZZZZZZ           777     7777  7777       777
        ZZZZZZZZZ   ZZZZ    Z               777     777    777       777
       ZZZZZZ       ZZZZ                    777     777    777       777
      ZZZZZ         ZZZZ                    777     777    777       777
     ZZZZZ          ZZZZZ    ZZZZ           777     777    777       777
     ZZZZ           ZZZZZ    ZZZZZ          777     777    777       777
     ZZZZ           ZZZZZ     ZZZZZ         777     777    777       777
     ZZZZ           ZZZZ       ZZZZZ        777     777    777       777
     ZZZZZ         ZZZZZ        ZZZZZ       777     777    777       777
      ZZZZZZ     ZZZZZZ          ZZZZZ      777     7777777777       777
       ZZZZZZZZZZZZZZZ            ZZZZ      777      77777777        777
         ZZZZZZZZZZZ               Z
            ZZZZZ

The friendly Operating System for IoT!

RIOT is a real-time multi-threading operating system that supports a range of
devices that are typically found in the Internet of Things (IoT):
8-bit, 16-bit and 32-bit microcontrollers.

RIOT is based on the following design principles: energy-efficiency, real-time
capabilities, small memory footprint, modularity, and uniform API access,
independent of the underlying hardware (this API offers partial POSIX
compliance).

RIOT is developed by an international open source community which is
independent of specific vendors (e.g. similarly to the Linux community).
RIOT is licensed with LGPLv2.1, a copyleft license which fosters
indirect business models around the free open-source software platform
provided by RIOT, e.g. it is possible to link closed-source code with the
LGPL code.

FEATURES
--------

RIOT is based on a microkernel architecture, and provides features including,
but not limited to:

* a preemptive, tickless scheduler with priorities
* flexible memory management
* high resolution, long-term timers
* support 100+ boards based on AVR, MSP430, ESP8266, ESP32, MIPS, RISC-V,
  ARM7 and ARM Cortex-M
* the native port allows to run RIOT as-is on Linux, BSD, and MacOS. Multiple
  instances of RIOT running on a single machine can also be interconnected via
  a simple virtual Ethernet bridge
* IPv6
* 6LoWPAN (RFC4944, RFC6282, and RFC6775)
* UDP
* RPL (storing mode, P2P mode)
* CoAP
* CCN-Lite
* Sigfox
* LoRaWAN


GETTING STARTED
---------------

* You want to start the RIOT? Just follow our
  `quickstart guide <https://doc.riot-os.org/index.html#the-quickest-start>`_
  or try this
  `tutorial <https://github.com/RIOT-OS/Tutorials/blob/master/README.md>`_.
  For specific toolchain installation, follow instructions in the
  `getting started <https://doc.riot-os.org/getting-started.html>`_ page.
* The RIOT API itself can be built from the code using doxygen. The latest
  version of the documentation is uploaded daily to
  `riot-os.org/api <https://riot-os.org/api>`_.

USING THE NATIVE PORT WITH NETWORKING
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you compile RIOT for the native cpu and include the ``netdev_tap`` module,
you can specify a network interface like this: ``PORT=tap0 make term``

SETTING UP A TAP NETWORK
""""""""""""""""""""""""

There is a shell script in ``RIOT/dist/tools/tapsetup`` called ``tapsetup`` which
you can use to create a network of tap interfaces.

*USAGE*

To create a bridge and two (or ``count`` at your option) tap interfaces: ::

    sudo ./dist/tools/tapsetup/tapsetup [-c [<count>]]

CONTRIBUTE
----------

To contribute something to RIOT, please refer to our
`contributing document <CONTRIBUTING.md>`_.

MAILING LISTS
-------------

* RIOT OS kernel developers list: `devel@riot-os.org <https://lists.riot-os.org/mailman/listinfo/devel>`_
* RIOT OS users list: `users@riot-os.org <https://lists.riot-os.org/mailman/listinfo/users>`_
* RIOT commits: `commits@riot-os.org <https://lists.riot-os.org/mailman/listinfo/commits>`_
* Github notifications: `notifications@riot-os.org <https://lists.riot-os.org/mailman/listinfo/notifications>`_

LICENSE
-------

* Most of the code developed by the RIOT community is licensed under the GNU
  Lesser General Public License (LGPL) version 2.1 as published by the Free
  Software Foundation.
* Some external sources, especially files developed by SICS are published under
  a separate license.

All code files contain licensing information.

For more information, see the RIOT website:

https://www.riot-os.org


.. |api| image:: https://img.shields.io/badge/docs-API-informational.svg
   :target: https://riot-os.org/api/
   :alt: API docs

.. |irc| image:: https://img.shields.io/badge/chat-IRC-brightgreen.svg
   :target: https://webchat.freenode.net?channels=%23riot-os
   :alt: IRC

.. |license| image:: https://img.shields.io/github/license/RIOT-OS/RIOT
   :target: https://github.com/RIOT-OS/RIOT/blob/master/LICENSE
   :alt: License

.. |CI| image:: https://ci.riot-os.org/RIOT-OS/RIOT/master/latest/badge.svg
   :target: https://ci.riot-os.org/nightlies.html#master
   :alt: Nightly CI status master

.. |matrix| image:: https://img.shields.io/badge/chat-Matrix-brightgreen.svg
   :target: https://matrix.to/#/#riot-os:matrix.org
   :alt: Matrix chat

.. |release| image:: https://img.shields.io/github/release/RIOT-OS/RIOT.svg
   :target: https://github.com/RIOT-OS/RIOT/releases/latest
   :alt: GitHub release

.. |stackoverflow| image:: https://img.shields.io/badge/stackoverflow-%5Briot--os%5D-yellow
   :target: https://stackoverflow.com/questions/tagged/riot-os
   :alt: Stackoverflow tag

.. |twitter| image:: https://img.shields.io/badge/social-Twitter-informational.svg
   :target: https://twitter.com/RIOT_OS
   :alt: Twitter

.. |wiki| image:: https://img.shields.io/badge/docs-Wiki-informational.svg
   :target: https://github.com/RIOT-OS/RIOT/wiki
   :alt: Wiki
