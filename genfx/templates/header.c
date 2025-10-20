/*
 *  File: header.c
 *
 *  NTS-3 kaoss pad kit generic effect unit header definition
 *
 */

#include "unit_genericfx.h"

#ifndef PROJECT_DEV_ID
 #define PROJECT_DEV_ID=(0x0U)
#endif

#ifndef PROJECT_UNIT_ID
 #define PROJECT_UNIT_ID=(0x0U)
#endif

const __unit_header genericfx_unit_header_t unit_header = {
  .common = {
    .header_size = sizeof(genericfx_unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_genericfx,
    .api = UNIT_API_VERSION,
    .dev_id = PROJECT_DEV_ID,
    .unit_id = PROJECT_UNIT_ID,
    .version = 0x00010000U,
    .name = "{{patch_name}}",
    .num_params = {{num_active_fixed_param + num_param}},
    .params = {
        // Format:
        // min, max, center, default, type, frac. bits, frac. mode, <reserved>, name
        {% if pitch is defined or pitch_note is defined %}{% raw %}
        {0, 1023, 480, 480, k_unit_param_type_midi_note, 0, 0, 0, {"PITCH"}},
        {% endraw %}
        {% endif %}
        {% if slfo is defined %}{% raw %}
        {0, 300, 100, 100, k_unit_param_type_hertz, 1, 1, 0, {"LFO Rate"}},
        {% endraw %}
        {% endif %}
        {% for i in range(1, 9 - num_active_fixed_param) %}
        {% set id = "param_id" ~ i %}
        {% if param[id] is defined %}
        {{'{' ~ param[id]['disp_min'] | int}}, {{param[id]['disp_max'] | int}}, {{param[id]['disp_default'] | int}}, {{param[id]['disp_default'] | int}}, k_unit_param_type_none, {{[param[id]['disp_frac'], 0] | max}}, 1, 0, {{ '{"' ~ param[id]['disp_name'] ~ '"}}' }}{% if not loop.last %},{{"\n"}}{% endif %}
        {% else %}{% raw %}
        {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}{% endraw %}{% if not loop.last %},{% endif %}
        {% endif %}
        {% if loop.last %}},{{"\n"}}{% endif %}
        {% endfor %}
  },
  .default_mappings = {
      {% if pitch is defined or pitch_note is defined %}{% raw %}
      {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 1023, 480},
      {% endraw %}
      {% endif %}
      {% if slfo is defined %}{% raw %}
      {k_genericfx_param_assign_none, k_genericfx_curve_exp, k_genericfx_curve_unipolar, 0, 300, 0},
      {% endraw %}
      {% endif %}
      {% for i in range(1, 9 - num_active_fixed_param) %}
      {% set id = "param_id" ~ i %}
      {% if param[id] is defined %}
      {{'{' ~ param[id]['assign']}}, {{param[id]['curve']}}, {{param[id]['polarity']}}, {{param[id]['disp_min'] | int}}, {{param[id]['disp_max'] | int}}, {{param[id]['disp_default'] | int ~ '}'}}{% if not loop.last %},{{"\n"}}{% endif %}
      {% else %}{% raw %}
      {k_genericfx_param_assign_none, k_genericfx_curve_linear, k_genericfx_curve_unipolar, 0, 0, 0}{% endraw %}{% if not loop.last %},{% endif %}
      {% endif %}
      {% if loop.last %}}{{"\n"}}{% endif %}
      {% endfor %}
};
