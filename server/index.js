// This file is part of switchbota (https://github.com/kendallgoto/switchbota/).
// Copyright (c) 2022 Kendall Goto.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 3.

// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

const express = require('express');
const app = express();
const port = 80;
const path = require('path');
const { https } = require('follow-redirects');
const fs = require('fs');
const crypto = require('crypto');
const dns = require('native-dns');
const async = require('async');
let LOCAL_IP = "0.0.0.0"
const PUBLIC_DNS = "8.8.8.8"

function startDns() {
	const dns_server = dns.createServer();
	let authority = { address: PUBLIC_DNS, port: 53, type: 'udp' };
	function dns_proxy(question, response, cb) {
		console.log('looking up ' + question.name);
		var request = dns.Request({
			question: question,
			server: authority,
			timeout: 1000
		});
		request.on('message', (err, msg) => {
			msg.answer.forEach(a => response.answer.push(a));
		});
		request.on('end', cb);
		request.send();
	}

	let entries = [{
		domain: ".*wohand\.com$",
		records: [{ type: "A", address: LOCAL_IP, ttl: 60 }]
	}];
	function handleDNSRequest(request, response) {
		console.log('request from', request.address.address, 'for', request.question[0].name);
		let f = []; // array of functions
		request.question.forEach(question => {
			let entry = entries.filter(r => new RegExp(r.domain, 'i').exec(question.name));
			if (entry.length) {
				entry[0].records.forEach(record => {
					record.name = question.name;
					record.ttl = record.ttl || 1800;
					response.answer.push(dns[record.type](record));
				});
			} else {
				f.push(cb => dns_proxy(question, response, cb));
			}
		});
		// do the proxying in parallel
		// when done, respond to the request by sending the response
		async.parallel(f, function () { response.send(); });
	}
	dns_server.on('request', handleDNSRequest);
	dns_server.on('listening', () => console.log('server listening on', dns_server.address()));
	dns_server.on('close', () => console.log('server closed', dns_server.address()));
	dns_server.on('error', (err, buff, req, res) => console.error(err.stack));
	dns_server.on('socketError', (err, socket) => console.error(err));
	dns_server.serve(53);
}

const args = process.argv.slice(2);
if (args.length) {
	LOCAL_IP = args[0]
	console.log("Starting with DNS server forwarding wohand.com to " + LOCAL_IP)
	startDns()
}
// You can temporarily set your router's DNS to the host this host's IP
// and it will correctly take care of wohand.com resolution.
const PAYLOAD_URL = 'https://github.com/arendst/Tasmota/releases/download/v11.1.0/tasmota32c3.factory.bin';
const PAYLOAD_PATH = path.join(__dirname, 'bin', 'payload.bin');
const PAYLOAD_BIN_MD5 = '14e7cc0d16e72da007727581520047d5';
const APP_URL = 'https://github.com/kendallgoto/switchbota/releases/latest/download/app.bin';
const APP_PATH = path.join(__dirname, 'bin', 'app.bin');
const APP_BIN_MD5 = 'cc9ec0df568b6e19da2096471ed8f531';
(async function () {

	function getFileHash(path) {
		const md5Hasher = crypto.createHash('md5');
		return md5Hasher.update(fs.readFileSync(path)).digest('hex');
	}

	function deleteInvalidFile(path) {
		try {
			console.log(`Deleting invalidated file: ${path}`);
			fs.unlinkSync(path);
		} catch (error) {
			console.log(`Error deleting invalidated file ${path}`);
			console.log(`Error: ${error}`);
		}
	}

	function invalidateFileOnBadMd5(path, md5) {
		if (fs.existsSync(path)) {
			if (getFileHash(path) != md5) {
				deleteInvalidFile(path);
			}
		}
	}

	async function download(path, url, md5) {
		if (!fs.existsSync(path)) {
			console.log('Downloading missing binary ' + url);
			https.get(url, (res) => {
				const result = fs.createWriteStream(path);
				res.pipe(result);
				result.on('finish', () => {
					result.close();
					if (getFileHash(path) != md5) {
						throw (`Download error: file ${path} does not match the expected md5 hash of ${md5}`);
					}
				});
			});
		}
	}

	invalidateFileOnBadMd5(PAYLOAD_PATH, PAYLOAD_BIN_MD5);
	invalidateFileOnBadMd5(APP_PATH, APP_BIN_MD5);

	download(PAYLOAD_PATH, PAYLOAD_URL, PAYLOAD_BIN_MD5);
	download(APP_PATH, APP_URL, APP_BIN_MD5);

	app.get('/payload.bin', (req, res) => {
		console.log(`${req.ip} - ${req.url}`);
		const file = path.join(__dirname, 'bin', 'payload.bin');
		res.setHeader('Content-Type', 'application/octet-stream');
		res.sendFile(file);
	})
	app.get('*', (req, res) => {
		console.log(`${req.ip} - ${req.url}`);
		const file = path.join(__dirname, 'bin', 'app.bin');
		res.setHeader('Content-Type', 'application/octet-stream');
		res.sendFile(file);
	});

	app.listen(port, () => {
		console.log(`Server listening on port ${port}`);
	})
})();
