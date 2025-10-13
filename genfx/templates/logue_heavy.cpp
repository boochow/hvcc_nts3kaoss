#include <errno.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <climits>

#include "unit_genericfx.h"

#include "utils/buffer_ops.h" // for buf_clr_f32()
#include "utils/int_math.h"   // for clipminmaxi32()

#include "osc_api.h"

#include "Heavy_{{patch_name}}.h"

#ifndef HV_MSGPOOLSIZE
 #define HV_MSGPOOLSIZE {{msg_pool_size_kb}}
#endif
#ifndef HV_INPUTQSIZE
 #define HV_INPUTQSIZE {{input_queue_size_kb}}
#endif
#ifndef HV_OUTPUTQSIZE
 #define HV_OUTPUTQSIZE {{output_queue_size_kb}}
#endif

static bool stop_unit_param;
static HeavyContextInterface* hvContext;

typedef enum {
    {% if pitch is defined or pitch_note is defined %}
    k_user_unit_param_pitch,
    {% endif %}
    k_user_unit_param_id1,
    k_user_unit_param_id2,
    k_user_unit_param_id3,
    k_user_unit_param_id4,
    k_user_unit_param_id5,
    k_user_unit_param_id6,
    k_user_unit_param_id7,
    k_user_unit_param_id8,
    k_num_user_unit_param_id
} user_unit_param_id_t;

static unit_runtime_desc_t s_desc;
static int32_t params[k_num_user_unit_param_id];

{% if touch_began is defined or touch_moved is defined or touch_ended is defined or touch_stationary is defined or touch_cancelled is defined %}
struct touch_event_t {
    bool dirty;
    uint8_t id;
    uint8_t phase;
    float x;
    float y;
};
static struct touch_event_t touch_event;
{% endif %}
{% if noteon_trig is defined %}
static bool noteon_trig_dirty;
{% endif %}
{% if noteoff_trig is defined %}
static bool noteoff_trig_dirty;
{% endif %}

{% for i in range(1, 9) %}
    {% set id = "param_id" ~ i %}
    {% if param[id] is defined %}
        {% if param[id]['type'] == 'int' %}
static int32_t {{param[id]['name']}};
        {% elif param[id]['type'] == 'float' %}
static float {{param[id]['name']}};
        {% endif %}
    {% endif %}
{% endfor %}
{% if num_param > 0 %}
static bool param_dirty[{{num_param}}];
{% endif %}
{% for key, entry in table.items() %}
static float * table_{{ key }};
static unsigned int table_{{ key }}_len;
{% endfor %}

__unit_callback int8_t unit_init(const unit_runtime_desc_t * desc)
{
    stop_unit_param = true;
    {% if touch_began is defined or touch_moved is defined or touch_ended is defined or touch_stationary is defined or touch_cancelled is defined %}
    touch_event.dirty = false;
    {% endif %}
    {% for i in range(1, 9) %}
      {% set id = "param_id" ~ i %}
      {% if param[id] is defined %}
        {% if param[id]['type'] == 'int' %}
    {{param[id]['name']}} = {{param[id]['default'] | int}};
        {% elif param[id]['type'] == 'float' %}
    {{param[id]['name']}} = {{param[id]['default']}};
        {% endif %}
    param_dirty[{{i - 1}}] = true;
      {% endif %}
    {% endfor %}

    if (!desc)
      return k_unit_err_undef;

    if (desc->target != unit_header.common.target)
      return k_unit_err_target;

    if (!UNIT_API_IS_COMPAT(desc->api))
      return k_unit_err_api_version;

    if (desc->samplerate != 48000)
      return k_unit_err_samplerate;

    if (desc->input_channels != 2 || desc->output_channels < {{num_output_channels}})
      return k_unit_err_geometry;

#if defined(UNIT_SDRAM_SIZE) && (UNIT_SDRAM_SIZE) > 0
    if (!desc->hooks.sdram_alloc)
      return k_unit_err_memory;
    init_sdram(desc->hooks.sdram_alloc);
#endif

#ifdef RENDER_HALF
    hvContext = hv_{{patch_name}}_new_with_options(24000, HV_MSGPOOLSIZE, HV_INPUTQSIZE, HV_OUTPUTQSIZE);
#else
    hvContext = hv_{{patch_name}}_new_with_options(48000, HV_MSGPOOLSIZE, HV_INPUTQSIZE, HV_OUTPUTQSIZE);
#endif
    {% for key, entry in table.items() %}
    table_{{ key }} = hv_table_getBuffer(hvContext, HV_{{patch_name|upper}}_TABLE_{{key|upper}});
    table_{{ key }}_len = hv_table_getLength(hvContext, HV_{{patch_name|upper}}_TABLE_{{key|upper}});
    {% endfor %}

    s_desc = *desc;

    return k_unit_err_none;
}

__unit_callback void unit_render(const float * in, float * out, uint32_t frames)
{
#ifdef RENDER_HALF
    float buffer[frames];
    static float last_buf_l = 0.f;
    static float last_buf_r = 0.f;
    float * __restrict p = buffer;
    float * __restrict y = out;
    const float * y_e = y + {{num_output_channels}} * frames;
#endif

    stop_unit_param = false;
    {% if pitch is defined %} 
    const float pitch = osc_w0f_for_note(params[0]>>3, (params[0] & 0x7)<<5) * k_samplerate;
    hv_sendFloatToReceiver(hvContext, HV_{{patch_name|upper}}_PARAM_IN_PITCH, pitch);
    {% endif %}
    {% if pitch_note is defined %}
    hv_sendFloatToReceiver(hvContext, HV_{{patch_name|upper}}_PARAM_IN_PITCH_NOTE, params[0]>>3);
    {% endif %}
    {% if touch_began is defined or touch_moved is defined or touch_ended is defined or touch_stationary is defined or touch_cancelled is defined %}
    if (touch_event.dirty) {
        touch_event.dirty = false;
        switch(touch_event.phase) {
        case k_unit_touch_phase_began:
            {% if touch_began is defined %}
            hv_sendMessageToReceiverV(hvContext, HV_{{patch_name|upper}}_PARAM_IN_TOUCH_BEGAN, 0, "ff", touch_event.x, touch_event.y);
            {% endif %}
            break;
        case k_unit_touch_phase_moved:
            {% if touch_moved is defined %}
            hv_sendMessageToReceiverV(hvContext, HV_{{patch_name|upper}}_PARAM_IN_TOUCH_MOVED, 0, "ff", touch_event.x, touch_event.y);
            {% endif %}
            break;
        case k_unit_touch_phase_ended:
            {% if touch_ended is defined %}
            hv_sendMessageToReceiverV(hvContext, HV_{{patch_name|upper}}_PARAM_IN_TOUCH_ENDED, 0, "ff", touch_event.x, touch_event.y);
            {% endif %}
            break;
        case k_unit_touch_phase_stationary:
            {% if touch_stationary is defined %}
            hv_sendMessageToReceiverV(hvContext, HV_{{patch_name|upper}}_PARAM_IN_TOUCH_STATIONARY, 0, "ff", touch_event.x, touch_event.y);
            {% endif %}
            break;
        case k_unit_touch_phase_cancelled:
            {% if touch_cancelled is defined %}
            hv_sendMessageToReceiverV(hvContext, HV_{{patch_name|upper}}_PARAM_IN_TOUCH_CANCELLED, 0, "ff", touch_event.x, touch_event.y);
            {% endif %}
            break;
        default:
            break;
        }
    }
    {% endif %}
    {% for i in range(1, 9 - num_fixed_param) %}
    {% set id = "param_id" ~ i %}
    {% if param[id] is defined %}
    if (param_dirty[{{i - 1}}]) {
        if (hv_sendFloatToReceiver(hvContext, HV_{{patch_name|upper}}_PARAM_IN_{{param[id]['name']|upper}}, {{param[id]['name']}})) {
            param_dirty[{{i - 1}}] = false;
        }
    }
    {% endif %}
    {% endfor %}
    {% for key, entry in table.items() %}
    {% if entry.type == "random" %}
    table_{{ key }}_len = hv_table_getLength(hvContext, HV_{{patch_name|upper}}_TABLE_{{key|upper}});
    for (int i = 0; i < table_{{ key }}_len ; i++) {
        table_{{ key }}[i] = fx_white();
    }
    {% endif %}
    {% endfor %}

    {% if noteon_trig is defined %}
    if (noteon_trig_dirty) {
        if (hv_sendBangToReceiver(hvContext, HV_{{patch_name|upper}}_PARAM_IN_NOTEON_TRIG)) {
            noteon_trig_dirty = false;
        }
    }
    {% endif %}
    {% if noteoff_trig is defined %}
    if (noteoff_trig_dirty) {
        if (hv_sendBangToReceiver(hvContext, HV_{{patch_name|upper}}_PARAM_IN_NOTEOFF_TRIG)) {
            noteoff_trig_dirty = false;
        }
    }
    {% endif %}

#ifdef RENDER_HALF
    {% if class_name == 'Nts3_bgfx' %}
    const unit_runtime_genericfx_context_t *ctxt = static_cast<const unit_runtime_genericfx_context_t *>(s_desc.hooks.runtime_context);
    hv_processInlineInterleaved(hvContext, (float *) ctxt->get_raw_input(), buffer, frames >> 1);
    {% else %}
    hv_processInlineInterleaved(hvContext, (float *) in, buffer, frames >> 1);
    {% endif %}
    for(int i = 0; y!= y_e; i++) {
        if (i & 1) {
            last_buf_l = *p++;
            last_buf_r = *p++;
            *(y++) = last_buf_l;
            *(y++) = last_buf_r;
        } else {
            *(y++) = (*p + last_buf_l) * 0.5;
            *(y++) = (*p + last_buf_r) * 0.5;
        }
    }
#else
    {% if class_name == 'Nts3_bgfx' %}
    const unit_runtime_genericfx_context_t *ctxt = static_cast<const unit_runtime_genericfx_context_t *>(s_desc.hooks.runtime_context);
    hv_processInlineInterleaved(hvContext, (float *) ctxt->get_raw_input(), out, frames);
    {% else %}
    hv_processInlineInterleaved(hvContext, (float *) in, out, frames);
    {% endif %}
#endif
    {% if num_output_channels == 1 %}
    if (s_desc.output_channels == 2) {
        float * p = out + frames;
        float * y = p + frames;
        for(; y > p ; ) {
            *(--y) = *(--p);
            *(--y) = *p;
        }
    }
    {% endif %}
}

__unit_callback void unit_set_param_value(uint8_t id, int32_t value)
{
    float knob_f = param_val_to_f32(value);
    float f;

    if (stop_unit_param) {
        return; // avoid all parameters to be zero'ed after unit_init()
    }
    params[id] = value;
    switch(id){
    {% for i in range(1, 9) %}
    {% set id = "param_id" ~ i %}
    {% if param[id] is defined %}
    case k_user_unit_{{id}}:
        {% if param[id]['type'] == 'int' %}
        {{param[id]['name']}} = value;
        param_dirty[{{i - 1}}] = true;
        {% elif param[id]['type'] == 'float' %}
        {{param[id]['name']}} = ({{param[id]['max'] - param[id]['min']}}) * {{ 10 ** (-param[id]['disp_frac']) }} * value / ({{param[id]['max'] - param[id]['min']}}) + ({{param[id]['min']}});
        param_dirty[{{i - 1}}] = true;
        {% endif %}
        break;
    {% endif %}
    {% endfor %}
    default:
      break;
    }
}

__unit_callback int32_t unit_get_param_value(uint8_t id) {
    return params[id];
}

static void note_on()
{
    {% if noteon_trig is defined %} 
    noteon_trig_dirty = true;
    {% endif %}
}

static void note_off()
{
    {% if noteoff_trig is defined %} 
    noteoff_trig_dirty = true;
    {% endif %}
}

__unit_callback void unit_teardown() {
}

__unit_callback void unit_reset() {
}

__unit_callback void unit_resume() {
}

__unit_callback void unit_suspend() {
}

__unit_callback const char * unit_get_param_str_value(uint8_t id, int32_t value\
) {
    return nullptr;
}

__unit_callback void unit_set_tempo(uint32_t tempo) {
}

__unit_callback void unit_tempo_4ppqn_tick(uint32_t counter) {
}

__unit_callback void unit_touch_event(uint8_t id, uint8_t phase, uint32_t x, uint32_t y) {
    {% if touch_began is defined or touch_moved is defined or touch_ended is defined or touch_stationary is defined or touch_cancelled is defined %}
    touch_event.dirty = true;
    touch_event.id = id;
    touch_event.phase = phase;
    touch_event.x = x;
    touch_event.y = y;
    {% endif %}
    {% if noteon_trig is defined or noteoff_trig is defined %}
    switch (phase) {
        {% if noteon_trig is defined %}
        case k_unit_touch_phase_began:
            note_on();
            break;
        {% endif %}
        {% if noteoff_trig is defined %}
        case k_unit_touch_phase_ended:
        case k_unit_touch_phase_cancelled:
            note_off();
            break;
        {% endif %}
        default:
            break;
    }
    {% endif %}
}
  
// dummy implementation for some starndard functions

int snprintf(char str[], size_t size, const char *format, ...) {
  return 0;
}

extern "C" void __cxa_pure_virtual() {
    while (1);  // do nothing
}

extern "C" void operator delete(void* ptr) noexcept {
    free(ptr);
}

extern "C" void operator delete[](void* ptr) noexcept {
    free(ptr);
}

extern "C" void* _sbrk(ptrdiff_t incr) {
    errno = ENOMEM;
    return (void*)-1;
}
