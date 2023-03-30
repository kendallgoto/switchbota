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


## Docker

### Docker Compose

There are 2 docker compose files. One uses host networking, with port `80` exposed. 

The other uses `2180` and is meant to be used in conjunction with nginx (or some other reverse proxy). 
See [How to configure a docker nginx reverse proxy](https://www.theserverside.com/blog/Coffee-Talk-Java-News-Stories-and-Opinions/Docker-Nginx-reverse-proxy-setup-example) for help and example on how to do this.
#### SAMPLE NGINX CONFIG
```

upstream switchbota_app {
  server 192.168.1.123:2180;
}

server {
	listen 80;
	server_name wohand.com 
	server_name www.wohand.com 
	server_name a.wohand.com; # wohand.com original DNS is set to CNAME a.wohand.com
	location / {
		resolver 192.168.1.1; # Your DNS

		proxy_http_version  1.1;
		proxy_set_header Host $host;
		proxy_set_header X-Real-IP $remote_addr;
		proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
		proxy_set_header Upgrade $http_upgrade;
		proxy_set_header Connection "upgrade";

		proxy_pass http://switchbota_app;
	}
}
```

### Docker Run
```
docker run --rm -p 80:80 ghcr.io/kendallgoto/switchbota/switchbota-server:latest
```

## Docker with Integrated DNS Server

Just like starting the server with node, you can pass the IP Address of the host 

```
docker run --rm --network=host ghcr.io/kendallgoto/switchbota/switchbota-server:latest 192.168.1.105
```

### Docker Compose
If you use the docker-compose you can uncomment the `command:` value and set that to the IP Address.
