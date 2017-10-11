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
 * Macros.h
 *
 *  Created on: 29-Mar-2017
 *      Author: abhijit
 */

#ifndef SRC_UTIL_MACROS_H_
#define SRC_UTIL_MACROS_H_

#define APP_LL_DEFINE(x_) x_ *next, *prev;
#define APP_LL_INIT_LIST next(NULL), prev(NULL)
#define APP_LL_RESET {next = NULL; prev = NULL;}
#define APP_LL_RESET_PTR(node) {node->next = NULL; node->prev = NULL;}

#define APP_LL_ADD_NEXT(node1, node2) \\
        { \\
            auto x = node1->next; \\
            node1->next = node2; \\
            node2->next = x; \\
        }

#define APP_LL_ADD_PREV(node1, node2) \\
        { \\
            auto x = node1->prev; \\
            node1->prev = node2; \\
            node2->prev = x; \\
        }

#define APP_LL_DEL_NEXT(node) node->next = node->next ? node->next->next : node->next

#endif /* SRC_UTIL_MACROS_H_ */
