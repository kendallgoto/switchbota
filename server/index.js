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

const PAYLOAD_URL = 'https://github.com/arendst/Tasmota/releases/download/v11.1.0/tasmota32c3.factory.bin';
const PAYLOAD_PATH = path.join(__dirname, 'bin', 'payload.bin');
const PAYLOAD_BIN_MD5 = '14e7cc0d16e72da007727581520047d5';
const APP_URL = 'https://github.com/kendallgoto/switchbota/releases/latest/download/app.bin';
const APP_PATH = path.join(__dirname, 'bin', 'app.bin');
const APP_BIN_MD5 = 'TODO-GET-UPDATED-APP-BIN-MD5';
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
		if (fs.existsSync(path))
		{
			if (getFileHash(path) != md5)
			{
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
					if (getFileHash(path) != md5)
					{
						throw(`Download error: file ${path} does not match the expected md5 hash of ${md5}`);
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
