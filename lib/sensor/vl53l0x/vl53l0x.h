#ifndef _TOFLIB_H_
#define _TOFLIB_H_
//
// Copyright (c) 2017 Larry Bank
// email: bitbank@pobox.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

//
// Initialize the VL53L0X sensor
// iChan: I2C channel (ignored on Pico, uses configured I2C)
// iAddr: I2C address of the sensor (usually 0x29)
// bLongRange: enable long range mode (0=standard, 1=long range)
// Returns: 1 on success, 0 on failure
//
int vl53l0x_init(int iChan, int iAddr, int bLongRange);

//
// Read the model and revision of the VL53L0X sensor
// model: pointer to store model ID
// revision: pointer to store revision ID
// Returns: 1 on success, 0 on failure
//
int vl53l0x_get_model(int *model, int *revision);

//
// Read the current distance measurement in millimeters
// Returns: distance in mm, or negative value on error
//
int vl53l0x_read_distance(void);

//
// Read distance continuously in millimeters
// Returns: distance in mm, or negative value on error
//
uint16_t vl53l0x_read_range_continuous_mm(void);

#endif // _TOFLIB_H_
