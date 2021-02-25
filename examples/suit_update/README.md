# Overview

This example shows how to integrate SUIT-compliant firmware updates into a
RIOT application. It implements basic support of the SUIT architecture using
the manifest format specified in
[draft-ietf-suit-manifest-09](https://tools.ietf.org/id/draft-ietf-suit-manifest-09.txt).

**WARNING**: This code should not be considered production ready for the time being.
             It has not seen much exposure or security auditing.

Table of contents:

- [Prerequisites][prerequisites]
  - [Ble][prerequisites-ble]
- [Setup][setup]
  - [Signing key management][key-management]
  - [Setup a wired device using ethos][setup-wired]
    - [Provision the device][setup-wired-provision]
    - [Configure the network][setup-wired-network]
  - [Alternative: Setup a wireless device behind a border router][setup-wireless]
    - [Provision the wireless device][setup-wireless-provision]
    - [Configure the wireless network][setup-wireless-network]
  - [Alternative: Setup a wireless ble device and Linux host][setup-wireless]
  - [Start aiocoap fileserver][start-aiocoap-fileserver]
- [Perform an update][update]
  - [Build and publish the firmware update][update-build-publish]
  - [Notify an update to the device][update-notify]
- [Testing without hardware][testing-without-hardware]
- [Detailed explanation][detailed-explanation]
- [Automatic test][test]

## Prerequisites
[prerequisites]: #Prerequisites

- Install python dependencies (only Python3.6 and later is supported):

      $ pip3 install --user cbor2 cryptography

- Install aiocoap from the source

      $ pip3 install --user --upgrade "git+https://github.com/chrysn/aiocoap#egg=aiocoap[all]"

  See the [aiocoap installation instructions](https://aiocoap.readthedocs.io/en/latest/installation.html)
  for more details.

- add `~/.local/bin` to PATH

  The aiocoap tools are installed to `~/.local/bin`. Either add
  "export `PATH=$PATH:~/.local/bin"` to your `~/.profile` and re-login, or execute
  that command *in every shell you use for this tutorial*.

- Clone this repository:

      $ git clone https://github.com/RIOT-OS/RIOT
      $ cd RIOT

- In all setup below, `ethos` (EThernet Over Serial) is used to provide an IP
  link between the host computer and a board.

  Just build `ethos` and `uhcpd` with the following commands:

      $ make -C dist/tools/ethos clean all
      $ make -C dist/tools/uhcpd clean all

  It is possible to interact with the device over it's serial terminal as usual
  using `make term`, but that requires an already set up tap interface.
  See [update] for more information.


### Ble
[prerequisites-ble]: #Ble

Make sure you fulfill the "Prerequisites" and "Preparing Linux" section in [README.ipv6-over-ble.md](../../pkg/nimble/README.ipv6-over-ble.md).

## Setup
[setup]: #Setup

### Key Management
[key-management]: #Key-management

SUIT keys consist of a private and a public key file, stored in `$(SUIT_KEY_DIR)`.
Similar to how ssh names its keyfiles, the public key filename equals the
private key file, but has an extra `.pub` appended.

`SUIT_KEY_DIR` defaults to the `keys/` folder at the top of a RIOT checkout.

If the chosen key doesn't exist, it will be generated automatically.
That step can be done manually using the `suit/genkey` target.

### Setup a wired device using ethos
[setup-wired]: #Setup-a-wired-device-using-ethos

#### Configure the network
[setup-wired-network]: #Configure-the-network

In one terminal, start:

    $ sudo dist/tools/ethos/setup_network.sh riot0 2001:db8::/64

This will create a tap interface called `riot0`, owned by the user. It will
also run an instance of uhcpcd, which starts serving the prefix
`2001:db8::/64`. Keep the shell open as long as you need the network.
Make sure to exit the "make term" instance from the next section *before*
exiting this, as otherwise the "riot0" interface doesn't get cleaned up
properly.

#### Provision the device
[setup-wired-provision]: #Provision-the-device

In order to get a SUIT capable firmware onto the node, run

    $ BOARD=samr21-xpro make -C examples/suit_update clean flash -j4

This command also generates the cryptographic keys (private/public) used to
sign and verify the manifest and images. See the "Key generation" section in
[SUIT detailed explanation][detailed-explanation] for details.

From another terminal on the host, add a routable address on the host `riot0`
interface:

    $ sudo ip address add 2001:db8::1/128 dev riot0

In another terminal, run:

    $ BOARD=samr21-xpro make -C examples/suit_update/ term

### Alternative: Setup a wireless device behind a border router
[setup-wireless]: #Setup-a-wireless-device-behind-a-border-router

If the workflow for updating using ethos is successful, you can try doing the
same over wireless network interfaces, by updating a node that is connected
wirelessly with a border router in between.

Depending on your device you can use BLE or 802.15.4.

#### Configure the wireless network

[setup-wireless-network]: #Configure-the-wireless-network

A wireless node has no direct connection to the Internet so a border router (BR)
between 802.15.4/BLE and Ethernet must be configured.
Any board providing a 802.15.4/BLE radio can be used as BR.

If configuring a BLE network when flashing the device include
`USEMODULE+=nimble_autoconn_ipsp` in the application Makefile, or prefix all
your make commands with it (for the BR as well as the device), e.g.:

     $ USEMODULE+=nimble_autoconn_ipsp make BOARD=<BR board>

Plug the BR board on the computer and flash the
[gnrc_border_router](https://github.com/RIOT-OS/RIOT/tree/master/examples/gnrc_border_router)
application on it:

    $ make BOARD=<BR board> -C examples/gnrc_border_router flash

In on terminal, start the network (assuming on the host the virtual port of the
board is `/dev/ttyACM0`):

    $ sudo ./dist/tools/ethos/start_network.sh /dev/ttyACM0 riot0 2001:db8::/64

Keep this terminal open.

From another terminal on the host, add a routable address on the host `riot0`
interface:

    $ sudo ip address add 2001:db8::1/128 dev riot0

#### Provision the wireless device
[setup-wireless-provision]: #Provision-the-wireless-device
First un-comment L28 in the application [Makefile](Makefile) so `gnrc_netdev_default`
is included in the build. In this scenario the node will be connected through a border
router. Ethos must be disabled in the firmware when building and flashing the firmware:

    $ USE_ETHOS=0 BOARD=samr21-xpro make -C examples/suit_update clean flash -j4

Open a serial terminal on the device to get its global address:

    $ USE_ETHOS=0 BOARD=samr21-xpro make -C examples/suit_update term

If the Border Router is already set up when opening the terminal you should get

    ...

    Iface  6  HWaddr: 0D:96  Channel: 26  Page: 0  NID: 0x23
            Long HWaddr: 79:7E:32:55:13:13:8D:96
             TX-Power: 0dBm  State: IDLE  max. Retrans.: 3  CSMA Retries: 4
            AUTOACK  ACK_REQ  CSMA  L2-PDU:102 MTU:1280  HL:64  RTR
            RTR_ADV  6LO  IPHC
            Source address length: 8
            Link type: wireless
            inet6 addr: fe80::7b7e:3255:1313:8d96  scope: link  VAL
            inet6 addr: 2001:db8::7b7e:3255:1313:8d96  scope: global  VAL
            inet6 group: ff02::2
            inet6 group: ff02::1
            inet6 group: ff02::1:ff17:dd59
            inet6 group: ff02::1:ff00:2

    suit_coap: started.

Here the global IPv6 is `2001:db8::7b7e:3255:1313:8d96`.
**The address will be different according to your device and the chosen prefix**.
In this case the RIOT node can be reached from the host using its global address:

    $ ping6 2001:db8::7b7e:3255:1313:8d96

_NOTE_: when using BLE the connection might take a little longer, and you might not
see the global address right away. But the global address will always consist of the
the prefix (`2001:db8::`) and the EUI64 suffix, in this case `7b7e:3255:1313:8d96`.

### Alternative: Setup a wireless ble device and Linux host

- Complete [Ble][prerequisites-ble].

- Provision the wireless ble device:

```
    $ CFLAGS=-DCONFIG_GNRC_IPV6_NIB_SLAAC=1 USEMODULE+=nimble_autoconn_ipsp USE_ETHOS=0 BOARD=nrf52dk make -C examples/suit_update clean flash -j4
```

- Open a serial terminal on the device to get its local address:

```
    $ USE_ETHOS=0 BOARD=nrf52dk make -C examples/suit_update term
```

    ...
        Iface  8  HWaddr: E4:DD:E0:8F:73:65
                  L2-PDU:1280 MTU:1280  HL:64  RTR
                  6LO  IPHC
                  Source address length: 6
                  Link type: wireless
                  inet6 addr: fe80::e4dd:e0ff:fe8f:7365  scope: local  VAL
                  inet6 group: ff02::2
                  inet6 group: ff02::1
                  inet6 group: ff02::1:ff8f:7365
    ...


**NOTE 2:** Currently, Linux does not support 6LoWPAN neighbor discovery (which
RIOT uses per default with BLE), so RIOT needs to be compiled to use stateless
address auto configuration (SLAAC) -> `CFLAGS=-DCONFIG_GNRC_IPV6_NIB_SLAAC=1`.

- Use `bluetoothctl` on Linux to scan for the device. Once `bluetoothctl` has
  started, issue `scan on` to start scanning. The default name for the RIOT
  device is set to `RIOT-autoconn`, so you should see it pop up. You can also
  use `devices` to list scanned devices.

      ...
      $ bluetoothctl
          Agent registered
          [bluetooth]# scan on
          Discovery started
          [CHG] Controller F4:5C:89:9F:AC:7A Discovering: yes
          [CHG] Device E4:DD:E0:8F:73:65 RSSI: -49
          [CHG] Device 43:1A:39:CD:39:B9 RSSI: -94
      ...
      ...
          [bluetooth]# devices
          Device F0:36:27:6B:F1:8F Decathlon Dual HR
          Device 69:B3:82:0B:73:C9 69-B3-82-0B-73-C9
          Device 43:1A:39:CD:39:B9 43-1A-39-CD-39-B9
          Device E4:DD:E0:8F:73:65 RIOT-autoconn
      ...

- Once you have the address, simply connect Linux to RIOT using the following
command:

      # Put your device address here...
      # Note: the 2 after the address denotes a BLE public random address, default
      #       used by `nimble_netif`
      echo "connect UU:VV:WW:XX:YY:ZZ 2" > /sys/kernel/debug/bluetooth/6lowpan_control

- Verify that the ble interface has been correctly created:


      $ ifconfig bt0

      ...
      bt0: flags=4161<UP,RUNNING,MULTICAST>  mtu 1280
              inet6 fe80::19:86ff:fe00:16ca  prefixlen 64  scopeid 0x20<link>
              unspec 00-19-86-00-16-CA-00-1E-00-00-00-00-00-00-00-00  txqueuelen 1000  (UNSPEC)
              RX packets 330  bytes 22891 (22.8 KB)
              RX errors 0  dropped 0  overruns 0  frame 0
              TX packets 354  bytes 30618 (30.6 KB)
              TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
      ...


- You should now be able to ping the device

      $ ping6 fe80::e4dd:e0ff:fe8f:7365%bt0

- **optional**: follow the guide for distributing a routable Prefix in
  [README.ipv6-over-ble.md](../../pkg/nimble/README.ipv6-over-ble.md).

If this was performed correctly then the `bt0` interface should now have a global
address:

    bt0: flags=4161<UP,RUNNING,MULTICAST>  mtu 1280
            inet6 2001:db8::19:86ff:fe00:16ca  prefixlen 64  scopeid 0x0<global>
            inet6 fe80::19:86ff:fe00:16ca  prefixlen 64  scopeid 0x20<link>
            inet6 2001:db8::b004:c58:891f:aa09  prefixlen 64  scopeid 0x0<global>
            unspec 00-19-86-00-16-CA-00-14-00-00-00-00-00-00-00-00  txqueuelen 1000  (UNSPEC)
            RX packets 3  bytes 120 (120.0 B)
            RX errors 0  dropped 0  overruns 0  frame 0
            TX packets 34  bytes 3585 (3.5 KB)
            TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

In this case the address to use for `SUIT_COAP_SERVER` can be either the EUI64
generated global address `[2001:db8::19:86ff:fe00:16ca]` or the random global address
`[2001:db8::b004:c58:891f:aa09]`.

If for some reason this didn't work, you can manually set up an address for
the subnet:

    $ sudo ip address add 2001:db8::1/64 dev bt0

In this case the address used for `SUIT_COAP_SERVER` should be [`2001:db8::1`].

Route traffic going towards your subnet through bt0:

    $ sudo route -A inet6 add 2001:db8::/64 dev bt0

In either case the address used for `SUIT_CLIENT` should be the suffix of the link
local address for that device (`e4dd:e0ff:fe8f:7365` in our examples) and the
distributed prefix, i.e.: `SUIT_CLIENT=[2001:db8::e4dd:e0ff:fe8f:7365]`

If this optional step is skipped then `SUIT_COAP_SERVER` will be
the link local address of the `bt0` interface and `SUIT_CLIENT` will be
the link local address of the device, with the interface specified. e.g:


      SUIT_COAP_SERVER=[fe80::19:86ff:fe00:16ca]
      SUIT_CLIENT=[fe80::e4dd:e0ff:fe8f:7365%bt0]

### Start aiocoap-fileserver
[Start-aiocoap-fileserver]: #start-aiocoap-fileserver

`aiocoap-fileserver` is used for hosting the firmwares available for updates.
Devices retrieve the new firmware using the CoAP protocol.

Start `aiocoap-fileserver`:

    $ mkdir -p coaproot
    $ aiocoap-fileserver coaproot

Keep the server running in the terminal.

## Perform an update
[update]: #Perform-an-update

### Build and publish the firmware update
[update-build-publish]: #Build-and-publish-the-firmware-update

Currently, the build system assumes that it can publish files by simply copying
them to a configurable folder.

For this example, aiocoap-fileserver serves the files via CoAP.

- To publish an update for a node in wired mode (behind ethos):

      $ BOARD=samr21-xpro SUIT_COAP_SERVER=[2001:db8::1] make -C examples/suit_update suit/publish

- To publish an update for a node in wireless mode (behind a border router):

      $ BOARD=samr21-xpro USE_ETHOS=0 SUIT_COAP_SERVER=[2001:db8::1] make -C examples/suit_update suit/publish

This publishes into the server a new firmware for a samr21-xpro board. You should
see 6 pairs of messages indicating where (filepath) the file was published and
the corresponding coap resource URI

    ...
    published "/home/francisco/workspace/RIOT/examples/suit_update/bin/samr21-xpro/suit_update-riot.suitv3_signed.1557135946.bin"
           as "coap://[2001:db8::1]/fw/samr21-xpro/suit_update-riot.suitv3_signed.1557135946.bin"
    published "/home/francisco/workspace/RIOT/examples/suit_update/bin/samr21-xpro/suit_update-riot.suitv3_signed.latest.bin"
           as "coap://[2001:db8::1]/fw/samr21-xpro/suit_update-riot.suitv3_signed.latest.bin"
    ...

### Notify an update to the device
[update-notify]: #Norify-an-update-to-the-device

If the network has been started with a standalone node, the RIOT node should be
reachable via link-local EUI64 address on the ethos interface, e.g:


    Iface  5  HWaddr: 02:BE:74:C0:2F:B9
            L2-PDU:1500 MTU:1500  HL:64  RTR
            RTR_ADV
            Source address length: 6
            Link type: wired
            inet6 addr: fe80::7b7e:3255:1313:8d96  scope: link  VAL
            inet6 addr: fe80::2  scope: link  VAL
            inet6 group: ff02::2
            inet6 group: ff02::1
            inet6 group: ff02::1:ffc0:2fb9
            inet6 group: ff02::1:ff00:2

the EUI64 link local address is `fe80::7b7e:3255:1313:8d96` and
SUIT_CLIENT=[fe80::7b7e:3255:1313:8d96%riot0].

If it was setup as a wireless device it will be reachable via its global
address, e.g:


    Iface  6  HWaddr: 0D:96  Channel: 26  Page: 0  NID: 0x23
            Long HWaddr: 79:7E:32:55:13:13:8D:96
             TX-Power: 0dBm  State: IDLE  max. Retrans.: 3  CSMA Retries: 4
            AUTOACK  ACK_REQ  CSMA  L2-PDU:102 MTU:1280  HL:64  RTR
            RTR_ADV  6LO  IPHC
            Source address length: 8
            Link type: wireless
            inet6 addr: fe80::7b7e:3255:1313:8d96  scope: link  VAL
            inet6 addr: 2001:db8::7b7e:3255:1313:8d96  scope: global  VAL
            inet6 group: ff02::2
            inet6 group: ff02::1
            inet6 group: ff02::1:ff17:dd59
            inet6 group: ff02::1:ff00:2


the global address is `2001:db8::7b7e:3255:1313:8d96` and
SUIT_CLIENT=[2001:db8::7b7e:3255:1313:8d96].

- In wired mode:

      $ SUIT_COAP_SERVER=[2001:db8::1] SUIT_CLIENT=[fe80::7b7e:3255:1313:8d96%riot] BOARD=samr21-xpro make -C examples/suit_update suit/notify

- In wireless mode:

      $ SUIT_COAP_SERVER=[2001:db8::1] SUIT_CLIENT=[2001:db8::7b7e:3255:1313:8d96] BOARD=samr21-xpro make -C examples/suit_update suit/notify


This notifies the node of a new available manifest. Once the notification is
received by the device, it fetches it.

If using `suit-v3` the node hangs for a couple of seconds when verifying the
signature:

    ....
    suit_coap: got manifest with size 470
    suit: verifying manifest signature
    ....

Once the signature is validated it continues validating other parts of the
manifest.
Among these validations it checks some condition like firmware offset position
in regards to the running slot to see witch firmware image to fetch.

    ....
    suit: validated manifest version
    )suit: validated sequence number
    )validating vendor ID
    Comparing 547d0d74-6d3a-5a92-9662-4881afd9407b to 547d0d74-6d3a-5a92-9662-4881afd9407b from manifest
    validating vendor ID: OK
    validating class id
    ....

Once the manifest validation is complete, the application fetches the image
and starts flashing.
This step takes some time to fetch and write to flash. A progress bar is
displayed during this step:

    ....
    Fetching firmware |█████████████            |  50%
    ....

Once the new image is written, a final validation is performed and, in case of
success, the application reboots on the new slot:

    Finalizing payload store
    Verifying image digest
    Starting digest verification against image
    Install correct payload
    Verifying image digest
    Starting digest verification against image
    Install correct payload
    Image magic_number: 0x544f4952
    Image Version: 0x5fa52bcc
    Image start address: 0x00201400
    Header chksum: 0x53bb3d33
    suit_coap: rebooting...

    main(): This is RIOT! (Version: <version xx>))
    RIOT SUIT update example application
    Running from slot 1
    ...

The slot number should have changed from after the application reboots.
You can do the publish-notify sequence several times to verify this.

## Testing Without Hardware
[testing-without-hardware]: #testing-without-hardware

The SUIT update example is compatible with the native application to try out the
update process. While it is not possible to update the running code in the
application, the workflow can still be used to update an in-memory storage.

### Description

When building the example application for the native target, the firmware update
capability is removed. Instead two in-memory slots are created that can be
updated with new payloads. These act as a demonstrator for the SUIT
capabilities.

The steps described here show how to use SUIT manifests to deliver content
updates to a RIOT instance. The full workflow is described, including the setup
of simple infrastructure.

The steps are as follow: First the network configuration is done. A CoAP server
is started to host the files for the RIOT instance. The necessary keys to sign
the manifest with are generated. After this the RIOT native instance is compiled
and launched. With this infrastructure running, the content and the manifest is
generated. Finally the RIOT instance is instructed to fetch the manifest and
update the storage location with the content.

### Workflow

While the above examples use make targets to create and submit the manifest,
this workflow aims to provide a better view of the SUIT manifest and signature
workflow. Because of this the steps below use the low level scripts to manually
creates a payload and manifest and sign it.

#### Setting up networking

To deliver the payload to the native instance, a network connection between a
coap server and the instance is required.

First a bridge with two tap devices is created:

```console
$ sudo dist/tools/tapsetup/tapsetup -c
```

This creates a bridge called `tapbr0` and a `tap0` and `tap1`. These last two
tap devices are used by native instances to inject and receive network packets
to and from.

On the bridge device `tapbr0` an routable IP address is added such as
`2001:db8::1/64`:

```console
$ sudo ip address add 2001:db8::1/64 dev tapbr0
```

#### Starting the CoAP server

As mentioned above, a CoAP server is required to allow the native instance to
retrieve the manifest and payload. The `aiocoap-fileserver` is used for this,
hosting files under the `coaproot` directory:

```console
$ aiocoap-fileserver coaproot
```

This should be left running in the background. A different directory can be used
if preferred.

#### Key generation

Public and private keys can be generated as described in the
[Signing key management][key-management] section. In this example the key is
stored in the `keys` directory.

#### Building the example

Before the natice instance can be started, it must be compiled first.
Compilation can be started from the root of your RIOT directory with:

```
$ make BOARD=native -C examples/suit_update
```

Then start the example with:

```console
$ make BOARD=native -C examples/suit_update term
```

This starts an instance of the suit_update example as a process on your
computer. It can be stopped by pressing `ctrl+c` from within the application.

#### Exploring the native instance

The native instance has two shell commands to inspect the storage backends for
the payloads.

- The `lsstorage` command shows the available storage locations:

```console
> lsstorage
lsstorage
RAM slot 0: ".ram.0"
RAM slot 1: ".ram.1"
```

As shown above, two storage locations are available, `.ram.0` and `.ram.1`.
While two slots are available, in this example only the content of the `.ram.0`
slot will be updated.

- The `storage-content` command can be used to display a hex dump command of one
  of the storage locations. It requires a location string, an offset and a
  number of bytes to print:
```console
> storage-content .ram.0 0 64
storage-content .ram.0 0 64

```
As the storage location is empty on boot, nothing is printed.

#### Generating the payload and manifest

To update the storage location we first need a payload. A trivial payload is
used in this example:

```console
$ echo "AABBCCDD" > coaproot/payload.bin
```

Make sure to store it in the directory selected for the CoAP file server.

Next, a manifest template is created. This manifest template is a JSON file that
acts as a template for the real SUIT manifest. Within RIOT, the script
`dist/tools/suit/gen_manifest.py` is used.

```console
$ dist/tools/suit/gen_manifest.py --urlroot coap://[2001:db8::1]/ --seqnr 1 -o suit.tmp coaproot/payload.bin:0:ram:0
```

This generates a suit manifest template with the sequence number set to `1`, a
payload that should be stored at slot offset zero in slot `.ram.0`. The url for
the payload starts with `coap://[fe80::4049:bfff:fe60:db09]/`. Make sure to
match these with the locations and IP addresses used on your own device.

SUIT supports a check for a slot offset. Within RIOT this is normally used to
distinguish between the different firmware slots on a device. As this is not
used on a native instance, it is set to zero here. The location within a SUIT
manifest is an array of path components. Which character is used to separate
these path components is out of the scope of the SUIT manifest. The
`gen_manifest.py` command uses colons (`:`) to separate these components.
Within the manifest this will show up as an array containing `[ "ram", "0" ]`.

The content of this template file should look like this:

```json
{
    "manifest-version": 1,
    "manifest-sequence-number": 1,
    "components": [
        {
            "install-id": [
                "ram",
                "0"
            ],
            "vendor-id": "547d0d746d3a5a9296624881afd9407b",
            "class-id": "bcc90984fe7d562bb4c9a24f26a3a9cd",
            "file": "coaproot/suit_test.bin",
            "uri": "coap://[fe80::4049:bfff:fe60:db09]/suit_test.bin",
            "bootable": false
        }
    ]
}
```

The manifest version indicates the SUIT manifest specification version numbers,
this will always be 1 for now. The sequence number is the monotonically
increasing anti-rollback counter.

Each component, or payload, also has a number of parameters. The install-id
indicates the unique path where this component must be installed.
The vendor and class ID are used in manifest conditionals to ensure that the
payload is valid for the device it is going to be installed in. It is generated
based on the UUID(v5) of `riot-os.org` and the board name (`native`).

The file and uri are used to generated the URL parameter and the digest in the
manifest. The bootable flag specifies if the manifest generator should instruct
the node to reboot after applying the update.

Generating the actual SUIT manifest from this is done with:

```console
$ dist/tools/suit/suit-manifest-generator/bin/suit-tool create -f suit -i suit.tmp -o coaproot/suit_manifest
```

This generates the manifest in SUIT CBOR format. The content can be inspected by
using the `parse` subcommand:

```console
$ dist/tools/suit/suit-manifest-generator/bin/suit-tool parse -m coaproot/suit_manifest
```

The manifest generated doesn't have an authentication wrapper, it is unsigned
and will not pass inspection on the device or RIOT instance. The manifest can be
signed with the `sign` subcommand together with the keys generated earlier.

```console
$ dist/tools/suit/suit-manifest-generator/bin/suit-tool sign -k keys/default.pem -m coaproot/suit_manifest -o coaproot/suit_manifest.signed
```

This generates an authentication to the manifest. This is visible when
inspecting with the `parse` subcommand. The URL to this signed manifest will be
submitted to the instance so it can retrieve it and in turn retrieve the
component payload specified by the manifest.

#### Updating the storage location

The update process is a two stage process where first the instance receives the
URL for the manifest. It will download the manifest and verify the content.
After the manifest is verified, it will proceed with executing the command
sequences in the manifest and download the payload when instructed to.

The URL for the manifest can be submitted to the host with any coap client. In
this example the `coap-client` from libcoap is used:

```console
$ coap-client coap://[2001:db8::2]/suit/trigger -m post -e 'coap://[2001:db8::1]/suit_manifest.signed'
```

The payload is the full URL to the signed manifest. The native instance should
respond on this by downloading and executing the manifest. If all went well, the
output of the native instance should look something like this:

```
suit: received URL: "coap://[2001:db8::1]/suit_manifest.signed"
suit_coap: trigger received
suit_coap: downloading "coap://[2001:db8::1]/suit_manifest.signed"
suit_coap: got manifest with size 278
suit: verifying manifest signature
suit: validated manifest version
Retrieved sequence number: 0
Manifest seq_no: 1, highest available: 0
suit: validated sequence number
Formatted component name: .ram.0
validating vendor ID
Comparing 547d0d74-6d3a-5a92-9662-4881afd9407b to 547d0d74-6d3a-5a92-9662-4881afd9407b from manifest
validating vendor ID: OK
validating class id
Comparing bcc90984-fe7d-562b-b4c9-a24f26a3a9cd to bcc90984-fe7d-562b-b4c9-a24f26a3a9cd from manifest
validating class id: OK
SUIT policy check OK.
Formatted component name: .ram.0
Fetching firmware |█████████████████████████| 100%
Finalizing payload store
Verifying image digest
Starting digest verification against image
Install correct payload
Verifying image digest
Starting digest verification against image
Install correct payload
```

The storage location can now be inspected using the built-in command. If the
same payload as suggested above was used, it should look like this:

```Console
> storage-content .ram.0 0 64
41414242434344440A
```

The process can be done multiple times with both slot `.ram.0` and `.ram.1` and
different payloads. Keep in mind that the sequence number is a strict
monotonically number and must be increased after every update.

## Detailed explanation
[detailed-explanation]: #Detailed-explanation

### Node

For the suit_update to work there are important modules that aren't normally built
in a RIOT application:

* riotboot
    * riotboot_flashwrite
* suit
    * suit_transport_coap

#### riotboot

To be able to receive updates, the firmware on the device needs a bootloader
that can decide from witch of the firmware images (new one and olds ones) to boot.

For suit updates you need at least two slots in the current conception on riotboot.
The flash memory will be divided in the following way:

```
|------------------------------- FLASH ------------------------------------------------------------|
|-RIOTBOOT_LEN-|------ RIOTBOOT_SLOT_SIZE (slot 0) ------|------ RIOTBOOT_SLOT_SIZE (slot 1) ------|
               |----- RIOTBOOT_HDR_LEN ------|           |----- RIOTBOOT_HDR_LEN ------|
 --------------------------------------------------------------------------------------------------|
|   riotboot   | riotboot_hdr_1 + filler (0) | slot_0_fw | riotboot_hdr_2 + filler (0) | slot_1_fw |
 --------------------------------------------------------------------------------------------------|
```

The riotboot part of the flash will not be changed during suit_updates but
be flashed a first time with at least one slot with suit_capable fw.

    $ BOARD=samr21-xpro make -C examples/suit_update clean flash

When calling make with the `flash` argument it will flash the bootloader
and then to slot0 a copy of the firmware you intend to build.

New images must be of course written to the inactive slot, the device mist be able
to boot from the previous image in case the update had some kind of error, eg:
the image corresponds to the wrong slot.

On boot the bootloader will check the `riotboot_hdr` and boot on the newest
image.

`riotboot_flashwrite` module is needed to be able to write the new firmware to
the inactive slot.

riotboot is not supported by all boards. The default board is `samr21-xpro`,
but any board supporting `riotboot`, `flashpage` and with 256kB of flash should
be able to run the demo.

#### suit

The suit module encloses all the other suit_related module. Formally this only
includes the `sys/suit` directory into the build system dirs.

- **suit_transport_coap**

To enable support for suit_updates over coap a new thread is created.
This thread will expose 4 suit related resources:

* /suit/slot/active: a resource that returns the number of their active slot
* /suit/slot/inactive: a resource that returns the number of their inactive slot
* /suit/trigger: this resource allows POST/PUT where the payload is assumed
tu be a url with the location of a manifest for a new firmware update on the
inactive slot.
* /suit/version: this resource is currently not implemented and return "NONE",
it should return the version of the application running on the device.

When a new manifest url is received on the trigger resource a message is resent
to the coap thread with the manifest's url. The thread will then fetch the
manifest by a block coap request to the specified url.

- **support for v3**

This includes v3 manifest support. When a url is received in the /suit/trigger
coap resource it will trigger a coap blockwise fetch of the manifest. When this
manifest is received it will be parsed. The signature of the manifest will be
verified and then the rest of the manifest content. If the received manifest is valid it
will extract the url for the firmware location from the manifest.

It will then fetch the firmware, write it to the inactive slot and reboot the device.
Digest validation is done once all the firmware is written to flash.
From there the bootloader takes over, verifying the slot riotboot_hdr and boots
from the newest image.

#### Key Generation

To sign the manifest and for the device to verify the manifest a pair of keys
must be generated. Note that this is done automatically when building an
updatable RIOT image with `riotboot` or `suit/publish` make targets.

This is simply done using the `suit/genkey` make target:

    $ BOARD=samr21-xpro make -C examples/suit_update suit/genkey

You will get this message in the terminal:

    Generated public key: 'a0fc7fe714d0c81edccc50c9e3d9e6f9c72cc68c28990f235ede38e4553b4724'

### Network

For connecting the device with the internet we are using ethos (a simple
ethernet over serial driver).

When executing $RIOTBASE/dist/tools/ethos:

    $ sudo ./start_network.sh /dev/ttyACM0 riot0 2001:db8::1/64

A tap interface named `riot0` is setup. `fe80::1/64` is set up as it's
link local address and `fd00:dead:beef::1/128` as the "lo" unique link local address.

Also `2001:db8::1/64` is configured- as a prefix for the network. It also sets-up
a route to the `2001:db8::1/64` subnet through `fe80::2`. Where `fe80::2` is the default
link local address of the UHCP interface.

Finally when:

    $ sudo ip address add 2001:db8::1/128 dev riot0

We are adding a routable address to the riot0 tap interface. The device can
now send messages to the the coap server through the riot0 tap interface. You could
use a different address for the coap server as long as you also add a routable
address, so:

    $ sudo ip address add $(SUIT_COAP_SERVER) dev riot0

When using a border router the same thing is happening although the node is no
longer reachable through its link local address but routed through to border router
so we can reach it with its global address.

NOTE: if we weren't using a local server you would need to have ipv6 support
on your network or use tunneling.

NOTE: using `fd00:dead:beef::1` as an address for the coap server would also
work and you wouldn't need to add a routable address to the tap interface since
a route to the loopback interface (`lo`) is already configured.

### Server and file system variables

The following variables are defined in makefiles/suit.inc.mk:

    SUIT_COAP_BASEPATH ?= firmware/$(APPLICATION)/$(BOARD)
    SUIT_COAP_SERVER ?= localhost
    SUIT_COAP_ROOT ?= coap://$(SUIT_COAP_SERVER)/$(SUIT_COAP_BASEPATH)
    SUIT_COAP_FSROOT ?= $(RIOTBASE)/coaproot
    SUIT_PUB_HDR ?= $(BINDIR)/riotbuild/public_key.h

The following convention is used when naming a manifest

    SUIT_MANIFEST ?= $(BINDIR_APP)-riot.suitv3.$(APP_VER).bin
    SUIT_MANIFEST_LATEST ?= $(BINDIR_APP)-riot.suitv3.latest.bin
    SUIT_MANIFEST_SIGNED ?= $(BINDIR_APP)-riot.suitv3_signed.$(APP_VER).bin
    SUIT_MANIFEST_SIGNED_LATEST ?= $(BINDIR_APP)-riot.suitv3_signed.latest.bin

The following default values are using for generating the manifest:

    SUIT_VENDOR ?= "riot-os.org"
    SUIT_SEQNR ?= $(APP_VER)
    SUIT_CLASS ?= $(BOARD)
    SUIT_KEY ?= default
    SUIT_KEY_DIR ?= $(RIOTBASE)/keys
    SUIT_SEC ?= $(SUIT_KEY_DIR)/$(SUIT_KEY).pem

All files (both slot binaries, both manifests, copies of manifests with
"latest" instead of `$APP_VER` in riotboot build) are copied into the folder
`$(SUIT_COAP_FSROOT)/$(SUIT_COAP_BASEPATH)`. The manifests contain URLs to
`$(SUIT_COAP_ROOT)/*` and are signed that way.

The whole tree under `$(SUIT_COAP_FSROOT)` is expected to be served via CoAP
under `$(SUIT_COAP_ROOT)`. This can be done by e.g., `aiocoap-fileserver $(SUIT_COAP_FSROOT)`.

### Makefile recipes

The following recipes are defined in makefiles/suit.inc.mk:

suit/manifest: creates a non signed and signed manifest, and also a latest tag for these.
    It uses following parameters:

    - $(SUIT_KEY): name of key to sign the manifest
    - $(SUIT_COAP_ROOT): coap root address
    - $(SUIT_CLASS)
    - $(SUIT_VERSION)
    - $(SUIT_VENDOR)

suit/publish: makes the suit manifest, `slot*` bin and publishes it to the
    aiocoap-fileserver

    1.- builds slot0 and slot1 bin's
    2.- builds manifest
    3.- creates $(SUIT_COAP_FSROOT)/$(SUIT_COAP_BASEPATH) directory
    4.- copy's binaries to $(SUIT_COAP_FSROOT)/$(SUIT_COAP_BASEPATH)
    - $(SUIT_COAP_ROOT): root url for the coap resources

suit/notify: triggers a device update, it sends two requests:

    1.- COAP get to check which slot is inactive on the device
    2.- COAP POST with the url where to fetch the latest manifest for
    the inactive slot

    - $(SUIT_CLIENT): define the client ipv6 address
    - $(SUIT_COAP_ROOT): root url for the coap resources
    - $(SUIT_NOTIFY_MANIFEST): name of the manifest to notify, `latest` by
    default.

suit/genkey: this recipe generates a ed25519 key to sign the manifest

**NOTE**: to plugin a new server you would only have to change the suit/publish
recipe, respecting or adjusting to the naming conventions.**

## Automatic test
[Automatic test]: #test

This applications ships with an automatic test. The test script itself expects
the application and bootloader to be flashed. It will then create two more
manifests with increasing version numbers and update twice, confirming after
each update that the newly flashed image is actually running.

To run the test,

- ensure the [prerequisites] are installed

- make sure aiocoap-fileserver is in $PATH

- compile and flash the application and bootloader:

```
    $ make -C examples/suit_update clean all flash -j4
```

- [set up the network][setup-wired-network] (in another shell):

```
    $ sudo dist/tools/ethos/setup_network.sh riot0 2001:db8::/64
```

- run the test:

```
    $ make -C examples/suit_update test
```
