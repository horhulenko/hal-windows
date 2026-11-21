// MIT License
//
// Copyright (c) 2019 Ruhr University Bochum, Chair for Embedded Security. All Rights reserved.
// Copyright (c) 2019 Marc Fyrbiak, Sebastian Wallat, Max Hoffmann ("ORIGINAL AUTHORS"). All rights reserved.
// Copyright (c) 2021 Max Planck Institute for Security and Privacy. All Rights reserved.
// Copyright (c) 2021 Jörn Langheinrich, Julian Speith, Nils Albartus, René Walendy, Simon Klix ("ORIGINAL AUTHORS"). All Rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include "hal_core/utilities/enums.h"

namespace hal
{
    /**
     * Defines the type of a pin.
     */
    enum class PinType
    {
        none,            /**< Default pin. **/
        power,           /**< Power pin. **/
        ground,          /**< Ground pin. **/
        lut,             /**< Pin that generates output from LUT initialization string. **/
        state,           /**< Pin that generates output from internal state. **/
        neg_state,       /**< Pin that generates output from negated internal state. **/
        clock,           /**< Clock pin. **/
        enable,          /**< Enable pin. **/
        set,             /**< Set/preset pin. **/
        reset,           /**< Reset/clear pin. **/
        data,            /**< Data pin. **/
        address,         /**< Address pin. **/
        io_pad,          /**< IO pad pin. **/
        select,          /**< Select pin. **/
        carry,           /**< Carry pin. **/
        sum,             /**< Sum pin. **/
        status,          /**< Status pin.*/
        error,           /**< Error pin.*/
        error_detection, /**< Error detection pin.*/
        done,            /**< Done pin.*/
        control          /**< Control pin.*/
    };

    template<>
    inline std::map<PinType, std::string> EnumStrings<PinType>::data = {{PinType::none, "none"},
                                                                 {PinType::power, "power"},
                                                                 {PinType::ground, "ground"},
                                                                 {PinType::lut, "lut"},
                                                                 {PinType::state, "state"},
                                                                 {PinType::neg_state, "neg_state"},
                                                                 {PinType::clock, "clock"},
                                                                 {PinType::enable, "enable"},
                                                                 {PinType::set, "set"},
                                                                 {PinType::reset, "reset"},
                                                                 {PinType::data, "data"},
                                                                 {PinType::address, "address"},
                                                                 {PinType::io_pad, "io_pad"},
                                                                 {PinType::select, "select"},
                                                                 {PinType::carry, "carry"},
                                                                 {PinType::sum, "sum"},
                                                                 {PinType::status, "status"},
                                                                 {PinType::error, "error"},
                                                                 {PinType::error_detection, "error_detection"},
                                                                 {PinType::done, "done"},
                                                                 {PinType::control, "control"}};

}    // namespace hal
