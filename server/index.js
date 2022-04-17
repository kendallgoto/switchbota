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

const PAYLOAD_URL = "https://github.com/tasmota/install/blob/0957b916b1484f850af843dd9e6d3733c0a4b095/firmware/unofficial/tasmota32c3_2M.factory.bin?raw=true";
const PAYLOAD_PATH = path.join(__dirname, 'bin', 'payload.bin');
const APP_URL = "https://github.com/kendallgoto/switchbota/releases/latest/download/app.bin";
const APP_PATH = path.join(__dirname, 'bin', 'app.bin');
(async function () {

	async function download(path, url) {
		if (!fs.existsSync(path)) {
			console.log("Downloading missing binary " + url);
			https.get(url, (res) => {
				const result = fs.createWriteStream(path);
				res.pipe(result);
				result.on('finish', () => {
					result.close();
				});
			});
		}
	}

	download(PAYLOAD_PATH, PAYLOAD_URL);
	download(APP_PATH, APP_URL);

	app.get('/payload.bin', (req, res) => {
		const file = path.join(__dirname, 'bin', 'payload.bin');
		res.setHeader('Content-Type', 'application/octet-stream');
		res.sendFile(file);
	})
	app.get('*', (req, res) => {
		const file = path.join(__dirname, 'bin', 'app.bin');
		res.setHeader('Content-Type', 'application/octet-stream');
		res.sendFile(file);
	});

	app.listen(port, () => {
		console.log(`Server listening on port ${port}`);
	})
})();
