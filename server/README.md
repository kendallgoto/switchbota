# switchbota-server
Creates a web server to spoof www.wohand.com requests

## Install
Run with Node -- binaries to serve will automatically be downloaded as needed.

```shell
npm i
node index.js
```

## Integrated DNS Server
If you can't control DNS mappings on your router, an integrated DNS MitM is included in `index.js`. To use it, start the script with the LAN IP address of your local machine specified on the command line:

```shell
node index.js 192.168.1.105
```

Where _192.168.1.105_ is the IP address of the system running the Node application. Then, in your router's DHCP settings, configure the default DNS server to similarly be _192.168.1.105_, or whatever IP is used before. Almost all routers allow you to configure the default DNS server via their configuration portals - please see the manual for yours for specific instructions.
