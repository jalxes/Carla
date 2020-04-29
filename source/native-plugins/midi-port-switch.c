/*
 * Carla Native Plugins
 * Copyright (C) 2012-2015 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "CarlaNative.h"
#include "CarlaMIDI.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// -----------------------------------------------------------------------

typedef struct {
    const NativeHostDescriptor* host;
    bool ouputPorts[MAX_MIDI_CHANNELS];
} MidiPortSwitchHandle;

// -----------------------------------------------------------------------

static NativePluginHandle midiportswitch_instantiate(const NativeHostDescriptor* host)
{
    MidiPortSwitchHandle* const handle = (MidiPortSwitchHandle*)malloc(sizeof(MidiPortSwitchHandle));

    if (handle == NULL)
        return NULL;

    handle->host = host;

    for (int i=MAX_MIDI_CHANNELS; --i>=0;)
        handle->ouputPorts[i] = 1.0f;

    return handle;
}

#define handlePtr ((MidiPortSwitchHandle*)handle)

static void midiportswitch_cleanup(NativePluginHandle handle)
{
    free(handlePtr);
}

static uint32_t midiportswitch_get_parameter_count(NativePluginHandle handle)
{
    return MAX_MIDI_CHANNELS;

    // unused
    (void)handle;
}

static const NativeParameter* midiportswitch_get_parameter_info(NativePluginHandle handle, uint32_t index)
{
    if (index > MAX_MIDI_CHANNELS)
        return NULL;

    static NativeParameter param;
    static const NativeParameterScalePoint scalePoints[2] = { { "Off", 0.0f }, { "On", 1.0f } };
    static char paramName[24];

    param.hints = NATIVE_PARAMETER_IS_ENABLED|NATIVE_PARAMETER_IS_AUTOMABLE|NATIVE_PARAMETER_IS_BOOLEAN|NATIVE_PARAMETER_USES_SCALEPOINTS;
    param.name  = paramName;
    param.unit  = NULL;
    param.ranges.def       = 1.0f;
    param.ranges.min       = 0.0f;
    param.ranges.max       = 1.0f;
    param.ranges.step      = 1.0f;
    param.ranges.stepSmall = 1.0f;
    param.ranges.stepLarge = 1.0f;
    param.scalePointCount  = 2;
    param.scalePoints      = scalePoints;

    snprintf(paramName, 24, "%u", index+1);

    return &param;

    // unused
    (void)handle;
}

static float midiportswitch_get_parameter_value(NativePluginHandle handle, uint32_t index)
{
    if (index > MAX_MIDI_CHANNELS)
        return 0.0f;

    return handlePtr->ouputPorts[index] ? 1.0f : 0.0f;
}

static void midiportswitch_set_parameter_value(NativePluginHandle handle, uint32_t index, float value)
{
    if (index > MAX_MIDI_CHANNELS)
        return;

    handlePtr->ouputPorts[index] = (value >= 0.5f);
}

static void midiportswitch_process(NativePluginHandle handle, float** inBuffer, float** outBuffer, uint32_t frames, const NativeMidiEvent* midiEvents, uint32_t midiEventCount)
{
    const NativeHostDescriptor* const host     = handlePtr->host;
    const bool*                 const ouputPorts = handlePtr->ouputPorts;
    NativeMidiEvent tmpEvent;

    for (uint32_t i=0; i < midiEventCount; ++i)
    {
        const NativeMidiEvent* const midiEvent = &midiEvents[i];

        const uint8_t status = (uint8_t)MIDI_GET_STATUS_FROM_DATA(midiEvent->data);

        if (!MIDI_IS_CHANNEL_MESSAGE(status)){
            // pass through all non-message events
            host->write_midi_event(host->handle, midiEvent);
            continue;
        }

        tmpEvent.time    = midiEvent->time;
        tmpEvent.data[0] = midiEvent->data[0];
        tmpEvent.data[1] = midiEvent->data[1];
        tmpEvent.data[2] = midiEvent->data[2];
        tmpEvent.data[3] = midiEvent->data[3];
        tmpEvent.size    = midiEvent->size;

        for (uint32_t port=0; port < MAX_MIDI_CHANNELS; ++port){
            if (!ouputPorts[port])
                continue;
            
            tmpEvent.port = port;
            host->write_midi_event(host->handle, &tmpEvent);
        }
    }

    return;

    // unused
    (void)inBuffer;
    (void)outBuffer;
    (void)frames;
}

// -----------------------------------------------------------------------

static const NativePluginDescriptor midiportswitchDesc = {
    .category  = NATIVE_PLUGIN_CATEGORY_UTILITY,
    .hints     = NATIVE_PLUGIN_IS_RTSAFE,
    .supports  = NATIVE_PLUGIN_SUPPORTS_EVERYTHING,
    .audioIns  = 0,
    .audioOuts = 0,
    .midiIns   = 1,
    .midiOuts  = MAX_MIDI_CHANNELS,
    .paramIns  = 0,
    .paramOuts = 0,
    .name      = "MIDI Channel Switch",
    .label     = "midiportswitch",
    .maker     = "jalxes",
    .copyright = "GNU GPL v2+",

    .instantiate = midiportswitch_instantiate,
    .cleanup     = midiportswitch_cleanup,

    .get_parameter_count = midiportswitch_get_parameter_count,
    .get_parameter_info  = midiportswitch_get_parameter_info,
    .get_parameter_value = midiportswitch_get_parameter_value,

    .get_midi_program_count = NULL,
    .get_midi_program_info  = NULL,

    .set_parameter_value = midiportswitch_set_parameter_value,
    .set_midi_program    = NULL,
    .set_custom_data     = NULL,

    .ui_show = NULL,
    .ui_idle = NULL,

    .ui_set_parameter_value = NULL,
    .ui_set_midi_program    = NULL,
    .ui_set_custom_data     = NULL,

    .activate   = NULL,
    .deactivate = NULL,
    .process    = midiportswitch_process,

    .get_state = NULL,
    .set_state = NULL,

    .dispatcher = NULL
};

// -----------------------------------------------------------------------

void carla_register_native_plugin_midiportswitch(void);

void carla_register_native_plugin_midiportswitch(void)
{
    carla_register_native_plugin(&midiportswitchDesc);
}

// -----------------------------------------------------------------------
