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

#pragma once

#define BINARY_URL "http://www.wohand.com/payload.bin"
#define BINARY_MD5 (unsigned char[]){ 0x14, 0x46, 0x0e, 0x76, 0xb2, 0x6d, 0x2c, 0x5d, 0xbe, 0xb5, 0x5e, 0xe7, 0xc7, 0x70, 0xb9, 0x19 }
#define OTA_BUF 2048
#define WRITE_SIZE 0x171000

#define FALLBACK_URL "http://www.wohand.com/app.bin"
#define FALLBACK_SSID "switchbota"
#define FALLBACK_PASS "switchbota"
