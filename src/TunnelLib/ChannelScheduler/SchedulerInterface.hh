/*
 * This is an implementation of Viscous protocol.
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
 * SchedulerInterface.hh
 *
 *  Created on: 22-Aug-2017
 *      Author: abhijit
 */

#ifndef SRC_TUNNELLIB_CHANNELSCHEDULER_SCHEDULERINTERFACE_HH_
#define SRC_TUNNELLIB_CHANNELSCHEDULER_SCHEDULERINTERFACE_HH_

namespace scheduler {

class SchedulerInterface{
public:
    virtual void notifyFreeCell(appInt8 chId) = 0;
    virtual void readyToSend(appInt8 chId, appInt ready = 1) = 0;
    virtual void notifyDeadChannel(appInt8 chId) = 0;
};

} // namespace scheduler

#endif /* SRC_TUNNELLIB_CHANNELSCHEDULER_SCHEDULERINTERFACE_HH_ */
