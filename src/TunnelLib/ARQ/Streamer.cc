/*
 * This is an implemetation of Viscous protocol.
 * Copyright (C) 2017  Abhijit Mondal
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


/*
 * Streamer.cpp
 *
 *  Created on: 23-Feb-2017
 *      Author: abhijit
 */

#include "Streamer.h"

namespace ARQ {

Streamer::Streamer(ReliabilityMod* streamObj, appInt16 flowId): obj(streamObj), flowId_(flowId), closed(FALSE){
}

Streamer::~Streamer() {
}

} /* namespace ARQ */
