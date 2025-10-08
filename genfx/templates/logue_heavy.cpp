#include <errno.h>
//#include <sys/types.h>
//#include <cstdlib>

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
    k_user_unit_param_id1,
    k_user_unit_param_id2,
    k_user_unit_param_id3,
    k_user_unit_param_id4,
    k_user_unit_param_id5,
    k_user_unit_param_id6,
    k_user_unit_param_id7,
    k_user_unit_param_id8,
    k_user_unit_param_id9,
    k_user_unit_param_id10,
    k_num_user_unit_param_id
} user_unit_param_id_t;

static unit_runtime_desc_t s_desc;
static int32_t params[k_num_user_unit_param_id];

/*
{% if noteon_trig is defined %}
static bool noteon_trig_dirty;
static uint8_t noteon_velocity;
{% endif %}
{% if noteoff_trig is defined %}
static bool noteoff_trig_dirty;
{% endif %}
*/

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
    {% for i in range(1, 9) %}
      {% set id = "param_id" ~ i %}
      {% if param[id] is defined %}
    {{param[id]['name']}} = {{param[id]['default']}};
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

    if (desc->input_channels != 2 || desc->output_channels != {{num_output_channels}}) 
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
        table_{{ key }}[i] = osc_white();
    }
    {% endif %}
    {% endfor %}

    /*
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
    */

#ifdef RENDER_HALF
    hv_processInlineInterleaved(hvContext, (float *) in, buffer, frames >> 1);
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
    hv_processInlineInterleaved(hvContext, (float *) in, out, frames);
#endif
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

/*
__unit_callback void unit_note_on(uint8_t note, uint8_t velo)
{
    {% if noteon_trig is defined %} 
    noteon_trig_dirty = true;
    noteon_velocity = velo;
    {% else %}
    (void) note, velo;
    {% endif %}
}

__unit_callback void unit_note_off(uint8_t note)
{
    {% if noteoff_trig is defined %} 
    noteoff_trig_dirty = true;
    {% else %}
    (void) note;
    {% endif %}
}
*/

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
    // Note: Touch x/y events are already mapped to specific parameters so there is usually there no need to set parameters from here.
    //       Audio source type effects, for instance, may require these events to trigger enveloppes and such.

    (void)id;
    (void)phase;
    (void)x;
    (void)y;

    // switch (phase) {
    // case k_unit_touch_phase_began:
    //   break;
    // case k_unit_touch_phase_moved:
    //   break;
    // case k_unit_touch_phase_ended:
    //   break;  
    // case k_unit_touch_phase_stationary:
    //   break;
    // case k_unit_touch_phase_cancelled:
    //   break; 
    // default:
    //   break;
    // }
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
