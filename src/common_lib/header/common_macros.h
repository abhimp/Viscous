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
 * common_macros.h
 *
 *  Created on: 26-Jun-2016
 *      Author: abhijit
 */

#ifndef COMMON_MACROS_H_
#define COMMON_MACROS_H_


#ifdef __cplusplus
extern "C" {
#endif

#define PP_ARG_N( \
         _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8,  _9, _10, \
        _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
        _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
        _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, \
        _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
        _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
        _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, \
        _71, _72, _73, _74, _75, _76, _77, _78, _79, _80, \
        _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, \
        _91, _92, _93, _94, _95, _96, _97, _98, _99, _100, \
        _101, _102, _103, _104, _105, _106, _107, _108, _109, _110, \
        _111, _112, _113, _114, _115, _116, _117, _118, _119, _120, \
        _121, _122, _123, _124, _125, _126, _127, N, ...) N
/* Note 63 is removed */
#define PP_RSEQ_N()                                        \
        126, 125, 124, 123, 122, 121, 120, \
        119, 118, 117, 116, 115, 114, 113, 112, 111, 110, \
        109, 108, 107, 106, 105, 104, 103, 102, 101, 100, \
        99, 98, 97, 96, 95, 94, 93, 92, 91, 90, \
        89, 88, 87, 86, 85, 84, 83, 82, 81, 80, \
        79, 78, 77, 76, 75, 74, 73, 72, 71, 70, \
        69, 68, 67, 66, 65, 64, 63, 62, 61, 60, \
        59, 58, 57, 56, 55, 54, 53, 52, 51, 50, \
        49, 48, 47, 46, 45, 44, 43, 42, 41, 40, \
        39, 38, 37, 36, 35, 34, 33, 32, 31, 30, \
        29, 28, 27, 26, 25, 24, 23, 22, 21, 20, \
        19, 18, 17, 16, 15, 14, 13, 12, 11, 10, \
        9, 8, 7, 6, 5, 4, 3, 2, 1, 0


#define PP_NARG_(...)    PP_ARG_N(__VA_ARGS__)

/* Note dummy first argument _ and ##__VA_ARGS__ instead of __VA_ARGS__ */
#define PP_NARG(...)     PP_NARG_(_, ##__VA_ARGS__, PP_RSEQ_N())


//Macros to iterate over elipses
#define APP_MACRO_FOR_EACH_0(...) YOU_CAN_NOT_HAVE_0_ARGUMENTS
#define APP_MACRO_FOR_EACH_1(_fn1, _fn2, _x, ...) _fn1(_x) _fn2(_x)
#define APP_MACRO_FOR_EACH_2(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_1(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_3(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_2(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_4(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_3(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_5(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_4(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_6(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_5(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_7(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_6(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_8(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_7(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_9(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_8(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_10(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_9(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_11(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_10(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_12(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_11(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_13(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_12(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_14(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_13(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_15(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_14(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_16(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_15(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_17(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_16(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_18(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_17(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_19(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_18(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_20(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_19(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_21(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_20(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_22(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_21(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_23(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_22(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_24(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_23(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_25(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_24(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_26(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_25(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_27(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_26(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_28(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_27(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_29(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_28(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_30(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_29(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_31(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_30(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_32(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_31(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_33(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_32(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_34(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_33(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_35(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_34(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_36(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_35(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_37(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_36(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_38(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_37(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_39(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_38(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_40(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_39(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_41(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_40(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_42(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_41(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_43(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_42(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_44(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_43(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_45(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_44(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_46(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_45(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_47(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_46(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_48(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_47(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_49(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_48(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_50(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_49(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_51(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_50(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_52(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_51(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_53(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_52(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_54(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_53(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_55(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_54(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_56(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_55(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_57(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_56(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_58(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_57(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_59(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_58(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_60(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_59(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_61(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_60(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_62(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_61(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_63(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_62(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_64(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_63(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_65(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_64(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_66(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_65(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_67(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_66(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_68(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_67(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_69(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_68(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_70(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_69(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_71(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_70(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_72(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_71(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_73(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_72(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_74(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_73(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_75(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_74(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_76(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_75(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_77(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_76(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_78(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_77(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_79(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_78(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_80(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_79(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_81(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_80(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_82(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_81(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_83(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_82(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_84(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_83(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_85(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_84(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_86(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_85(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_87(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_86(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_88(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_87(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_89(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_88(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_90(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_89(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_91(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_90(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_92(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_91(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_93(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_92(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_94(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_93(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_95(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_94(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_96(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_95(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_97(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_96(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_98(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_97(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_99(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_98(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_100(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_99(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_101(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_100(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_102(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_101(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_103(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_102(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_104(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_103(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_105(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_104(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_106(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_105(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_107(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_106(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_108(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_107(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_109(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_108(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_110(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_109(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_111(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_110(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_112(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_111(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_113(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_112(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_114(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_113(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_115(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_114(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_116(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_115(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_117(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_116(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_118(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_117(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_119(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_118(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_120(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_119(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_121(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_120(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_122(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_121(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_123(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_122(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_124(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_123(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_125(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_124(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_126(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_125(_fn1, _fn2, __VA_ARGS__) _fn2(_x)
#define APP_MACRO_FOR_EACH_127(_fn1, _fn2, _x, ...) _fn1(_x) APP_MACRO_FOR_EACH_126(_fn1, _fn2, __VA_ARGS__) _fn2(_x)

#define APP_MACRO_DUMMY(_x)
#define APP_MACRO_FOR_EACH_(_fn1, _fn2, N, ...) APP_MACRO_FOR_EACH_##N(_fn1, _fn2, __VA_ARGS__)
#define APP_MACRO_FOR_EACH_N(...) APP_MACRO_FOR_EACH_(__VA_ARGS__)
#define APP_MACRO_FOR_EACH(_fn1, _fn2, ...) APP_MACRO_FOR_EACH_N(_fn1, _fn2, PP_NARG(__VA_ARGS__), __VA_ARGS__)


#ifdef __cplusplus
}
#endif

#endif /* COMMON_MACROS_H_ */
