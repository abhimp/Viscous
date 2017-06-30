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
 * DynamicArray.hh
 *
 *  Created on: 29-Jan-2017
 *      Author: abhijit
 */
#include <common.h>

#ifndef SRC_UTIL_DYNAMICARRAY_HPP_
#define SRC_UTIL_DYNAMICARRAY_HPP_

#define APP_DARRAY_PTR(x) darray_##x
#define APP_DARRAY_CAPA(x) darray_##x##_capa

#define APP_DARRAY_DECLARE(ptr, type) \
    type *APP_DARRAY_PTR(ptr); \
    appInt16 APP_DARRAY_CAPA(ptr);

#define APP_DARRAY_INIT(ptr) \
        APP_DARRAY_PTR(ptr)(NULL), APP_DARRAY_CAPA(ptr)(0)

#define APP_DARRAY_RESET(ptr) \
    {APP_DARRAY_PTR(ptr) = NULL; APP_DARRAY_CAPA(ptr) = 0;}

#define APP_DARRAY_HAVE_ID(ptr, id) \
        (APP_DARRAY_PTR(ptr) and APP_DARRAY_CAPA(ptr) > id)

#define APP_DARRAY_GET_VALUE(ptr, type, id, x) \
        if(APP_DARRAY_PTR(ptr) and APP_DARRAY_CAPA(ptr) > id) \
            x = APP_DARRAY_PTR(ptr)[id];

#define APP_DARRAY_SET_VALUE(ptr, type, id, x) \
        if(APP_DARRAY_PTR(ptr) == NULL or APP_DARRAY_CAPA(ptr) <= id){ \
            APP_DARRAY_PTR(ptr) = (type *) appRealloc(APP_DARRAY_PTR(ptr), sizeof(type) * (id+1)); \
            APP_ASSERT(APP_DARRAY_PTR(ptr)); \
            memset(APP_DARRAY_PTR(ptr) + APP_DARRAY_CAPA(ptr), 0, sizeof(type)*(id + 1 - APP_DARRAY_CAPA(ptr))); \
        }\
        APP_DARRAY_PTR(ptr)[id] = x;


#endif /* SRC_UTIL_DYNAMICARRAY_HPP_ */
