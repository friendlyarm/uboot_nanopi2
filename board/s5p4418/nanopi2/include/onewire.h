/*
 * Copyright (C) Guangzhou FriendlyARM Computer Tech. Co., Ltd.
 * (http://www.friendlyarm.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 */

#ifndef __ONE_WIRE_H__
#define __ONE_WIRE_H__

#define CFG_ONEWIRE_IO		(PAD_GPIO_C + 15)

extern void onewire_init(void);
extern int  onewire_get_info(unsigned char *lcd, unsigned short *fw_ver);
extern int  onewire_get_lcd_id(void);
extern int  onewire_set_backlight(int brightness);

#endif /* __ONE_WIRE_H__ */
