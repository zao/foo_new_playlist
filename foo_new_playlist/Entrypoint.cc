#include <SDK/foobar2000.h>

#pragma comment(lib, "foobar2000_component_client.lib")
#pragma comment(lib, "foobar2000_sdk.lib")
#pragma comment(lib, "pfc.lib")
#pragma comment(lib, "shared.lib")
#pragma comment(lib, "Shlwapi.lib")

DECLARE_COMPONENT_VERSION("New Playlist", "1.0", "zao");
VALIDATE_COMPONENT_FILENAME("foo_new_playlist.dll");

// {D753F438-37C6-406B-A52A-0A7161AF6E3A}
static GUID const g_pattern_guid = {
  0xd753f438,
  0x37c6,
  0x406b,
  { 0xa5, 0x2a, 0xa, 0x71, 0x61, 0xaf, 0x6e, 0x3a }
};

static advconfig_string_factory g_pattern_value(
  "New playlist pattern",
  g_pattern_guid,
  advconfig_entry::guid_branch_tools,
  0.0,
  "New Playlist[ '('%maybe_counter%')']");

struct Commands : mainmenu_commands
{
  t_uint32 get_command_count() override { return 1; }

  GUID get_command(t_uint32 p_index) override
  {
    // {8AF7E67F-F298-4F76-B899-5D353156043D}
    static GUID const s_guid = {
      0x8af7e67f,
      0xf298,
      0x4f76,
      { 0xb8, 0x99, 0x5d, 0x35, 0x31, 0x56, 0x4, 0x3d }
    };
    return s_guid;
  }

  void get_name(t_uint32 p_index, pfc::string_base& p_out) override
  {
    p_out = "New named playlist";
  }

  bool get_description(t_uint32 p_index, pfc::string_base& p_out) override
  {
    p_out = "Creates a new, empty playlist with a custom name, see advanced "
            "configuration for format.";
    return true;
  }

  GUID get_parent() override { return mainmenu_groups::file_playlist; }

  struct Hook : titleformat_hook
  {
    uint32_t counter = 1;
    bool process_field(titleformat_text_out* p_out,
                       const char* p_name,
                       t_size p_name_length,
                       bool& p_found_flag) override
    {
      if (stricmp(p_name, "maybe_counter") == 0) {
        if (counter == 1) {
          return false;
        }
        p_found_flag = true;
        p_out->write_int(titleformat_inputtypes::unknown, counter);
        return true;
      }
      else if (stricmp(p_name, "always_counter") == 0) {
        p_found_flag = true;
        p_out->write_int(titleformat_inputtypes::unknown, counter);
        return true;
      }
      return false;
    }

    bool process_function(titleformat_text_out* p_out,
                          const char* p_name,
                          t_size p_name_length,
                          titleformat_hook_function_params* p_params,
                          bool& p_found_flag) override
    {
      return false;
    }
  };

  void execute(t_uint32 p_index,
               service_ptr_t<service_base> p_callback) override
  {
    Hook hook;
    static_api_ptr_t<playlist_manager> pm;

    static_api_ptr_t<titleformat_compiler> tfm;

    service_ptr_t<titleformat_object> tfo;
    pfc::string8 pattern;
    g_pattern_value.get(pattern);
    if (!tfm->compile(tfo, pattern.c_str())) {
      return;
    }

    playable_location_impl loc;
    file_info_impl info;
    pfc::string8 last_name;
    size_t playlist_count = pm->get_playlist_count();
    while (1) {
      pfc::string8 name;
      tfo->run_hook(loc, &info, &hook, name, nullptr);
      if (last_name == name) {
        console::warning("Pattern evaluates to the same name repeatedly, consider using %maybe_counter% or %always_counter%.");
        return;
      }
      last_name = name;
      t_size idx = pm->find_or_create_playlist(name.c_str());
      if (idx >= playlist_count) {
        pm->set_active_playlist(idx);
        break;
      }
      hook.counter += 1;
    }
  }
};

static service_factory_t<Commands> g_commands;